// 引入模块
const grpc = require('@grpc/grpc-js');
const message_proto = require('./proto');
const const_module = require('./const');
const {v4:uuidv4} = require('uuid');
const emailModule = require('./email')
const redis_module = require('./redis')

// 异步函数：获取验证码
async function GetVerifyCode(call, callback) {
    console.log("email is ", call.request.email)
    try{
        // 从 Redis 中查询验证码
        let query_res = await redis_module.GetRedis(const_module.code_prefix+call.request.email);
        console.log("query_res is ", query_res)
        let uniqueId = query_res;
        if(query_res ==null){         // 如果查询结果为空
            uniqueId = uuidv4();        // 生成新的 UUID
            if (uniqueId.length > 4) {  // 截取前4位
                uniqueId = uniqueId.substring(0, 4);
            } 
            // 将验证码存入 Redis 并设置过期时间为600秒
            let bres = await redis_module.SetRedisExpire(const_module.code_prefix+call.request.email, uniqueId,600)
            if(!bres){                  // 如果存储失败
                callback(null, { email:  call.request.email,
                    error:const_module.Errors.RedisErr
                });
                return;
            }
        }
        // 设置邮件选项
        console.log("uniqueId is ", uniqueId)
        let text_str =  '您的验证码为'+ uniqueId +'请三分钟内完成注册'
        //发送邮件
        let mailOptions = {
            from: '1273106078@qq.com',
            to: call.request.email,
            subject: '验证码',
            text: text_str,
        };
        let send_res = await emailModule.SendMail(mailOptions);
        console.log("send res is ", send_res)
        callback(null, { email:  call.request.email,
            error:const_module.Errors.Success
        }); 

        // uniqueId = uuidv4();
        // console.log("uniqueId is ", uniqueId)
        // let text_str =  '您的验证码为'+ uniqueId +'请三分钟内完成注册'
        // //发送邮件
        // let mailOptions = {
        //     from: '1273106078@qq.com',
        //     to: call.request.email,
        //     subject: '验证码',
        //     text: text_str,
        // };
        // let send_res = await emailModule.SendMail(mailOptions);
        // console.log("send res is ", send_res)
        // callback(null, { email:  call.request.email,
        //     error:const_module.Errors.Success
        // }); 

    }catch(error){
        console.log("catch error is ", error)
        callback(null, { email:  call.request.email,
            error:const_module.Errors.Exception
        }); 
    }
}
//启动 gRPC 服务器
function main() {
    var server = new grpc.Server()
    server.addService(message_proto.VerifyService.service, { GetVerifyCode: GetVerifyCode })
    server.bindAsync('0.0.0.0:50051', grpc.ServerCredentials.createInsecure(), () => {
        //server.start()
        console.log('grpc server started')        
    })
}
main()
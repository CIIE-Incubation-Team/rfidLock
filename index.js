import express, { urlencoded, json } from 'express';
import { MongoClient, ServerApiVersion } from "mongodb"
import crypto from 'crypto'

const ConnectMongo = async () => {
    const uri = "mongodb+srv://aladsss:lpacafcs@unogame.oxplv.mongodb.net/myFirstDatabase?retryWrites=true&w=majority";
    const client = new MongoClient(uri, { useNewUrlParser: true, useUnifiedTopology: true, serverApi: ServerApiVersion.v1 });
    return await client.connect()
}

function decrypt(cipherText, key, outputEncoding = "utf8") {
    cipherText.split(',').splice(cipherText.length-1,1)
    let text = Buffer.from(cipherText)
    const cipher = crypto.createDecipheriv("aes-128-ecb", key);
    return Buffer.concat([cipher.update(text)]).toString(outputEncoding);
}

const key = Buffer.from([99,  58,  95,  49, 253, 54, 136, 144, 194, 151, 184, 84, 159,  59,  36, 178])

const app = express()
const client = await ConnectMongo()

app.use(urlencoded({extended:true}))
app.use(json())

app.get('/',(req,res,next)=>{
    res.json({"Status":"Working"}).status(200)
})

app.post('/authOpen',async (req,res,next)=>{
    let enced = req.body
    console.log(enced)
    let user
    let password
    if(enced['enc']){
        user = decrypt(enced['user'],key,null)
        password = decrypt(enced['password'],key,null)
    } else {
        user = enced['user']
        password = enced['password']
    }
    console.log(user)
    console.log(password)
    let resp = await client.db('rfidLock').collection('userData').find({user:user}).toArray()
    console.log(resp)

    if(resp[0]['password'] === password){
        res.json({'status':'open'}).status(200)
    } else {
        res.json({'status':'lock'}).status(200)
    }

    await client.db('rfidLock').collection('userLog').insertOne({"user":user,"timestamp":(new Date()).toDateString()})
})

app.listen(process.env.PORT || 3000)
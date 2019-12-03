# HALLYM[System Programming]
# Term Project
# 1:N Async Chatting by C

강동우 : Applicate encrypt algorithm to program and delvelop chatting server and client

허재웅 : Develop chatting server and client

------------------------------------------------------------------------------

Clients send their messages to Server then Server broadcasts clients' messages

Server doesn't save the messages at all

Server checks new client's ip address

Server counts clients' number



Reference : http://ftp.linux.co.kr/pub/DOCS/Linux_Network_&_System_Programming%B1%B3%C0%E7.pdf

------------------------------------------------------------------------------ 2019.11.20

Use SHA-256 algorithm to encrypt the admin's password

Someone who open the server can login as Admin

Admin can kick other users who talk aggressively

Admin including users can see the list who participating chatting room



Reference : http://blog.naver.com/PostView.nhn?blogId=ultract2&logNo=110167519084

------------------------------------------------------------------------------ 2019.12.02

# How to use it

If you want to open the server
./server port password

If you want to join the chatting room
./client IPaddress port name password

When you want to see the list who participating chatting room
/list

When you want to kick someone who talk aggressively(ONLY Admin)
/kick name

------------------------------------------------------------------------------

# Why do we need this?

So many people using KakaoTalk,
but we can't know whether they save our messages
and there are so many advertisements in KakaoTalk program

BUT this program
- There is no advertisement
- Anyone can open the server
- Share the password with friends or organizations before the open the server
- Password will protected by SHA256
- Server does not save the messages at all
- Easy to management

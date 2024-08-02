#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/uio.h>
#include "chat_class.cpp"

#define BUF_SIZE 1024
#define MAX_CLNT 256

void *handle_clnt(void *arg);
void send_msg(char *msg, int len, string nick, string want);
void send_groupmsg(char *msg, int len, string nick, string want);
void send_msg1(char *msg, int len, int clnt_sock, int cl_sock);
void send_groupmsg1(char *msg, int len, int *group_cl, int p);
void error_handling(std::string msg);
int requestlist(string u_id, int clnt_sock);
int friendlist(string u_id, int clnt_sock);
void Client_list(string id, int clnt_sock);
int clnt(string nick);
void chatlist(string nick, int clnt_sock, string type);
void rand_msg(const char *msg, int len);

struct Client
{
    int cl_sock;    // 클라이언트 소켓함
    string cl_nick; // 클라이언트 닉네임
} cl;

vector<struct Client> client_info;
int cl_sock = 0;
int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutx;
vector<int> ranchat;
int r_count = 0;

int main(int argc, char *argv[])
{
    int serv_sock, clnt_sock;
    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    pthread_t t_id;
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);
        exit(1);
    }

    pthread_mutex_init(&mutx, NULL);
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

        pthread_mutex_lock(&mutx);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutx);

        cout << clnt_sock << endl;

        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
        pthread_detach(t_id);
        printf("Connectd client IP: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }
    close(serv_sock);
    return 0;
}

void *handle_clnt(void *arg)
{
    chat c("OPERATOR", "1234");
    int clnt_sock = *((int *)arg);
    int str_len = 0, i;
    char msg[BUF_SIZE];
    char recv_msg[BUF_SIZE];
    char sendmsg[BUF_SIZE];
    int count = 1;
    while (1)
    {
        recv_msg[0] = 0;
        while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1)
            error_handling("read() error");
        recv_msg[str_len] = 0;
        std::string choice(recv_msg);
        if (choice == "q" || choice == "Q")
            break;
        else if (choice == "1" || choice == "로그인")
        {

            // id 수신
            while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1)
                error_handling("read() error");
            recv_msg[str_len] = 0;
            std::string id(recv_msg);
            // pw 수신
            while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1)
                error_handling("read() error");
            recv_msg[str_len] = 0;
            std::string pw(recv_msg);
            std::string logreturn;
            if (count < 6)
            {
                pthread_mutex_lock(&mutx); // 뮤텍스 시작
                string enter = c.LOGIN(id, pw);
                pthread_mutex_unlock(&mutx); // 뮤텍스 종료
                strcpy(sendmsg, enter.c_str());
                sendmsg[strlen(enter.c_str())] = 0;
                // 로그인 결과 송신
                write(clnt_sock, sendmsg, strlen(sendmsg));
                logreturn = enter;
            }
            else
            { // 고유번호 수신(string으로 수신 int로 변환)
                while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1)
                    error_handling("read() error");
                recv_msg[str_len] = 0;
                string str_uid(recv_msg);
                int U_id = stoi(str_uid);
                pthread_mutex_lock(&mutx);
                string enter = c.LOGIN(id, pw, U_id);
                pthread_mutex_unlock(&mutx);
                // 로그인 결과 송신
                write(clnt_sock, enter.c_str(), strlen(enter.c_str()));
                logreturn = enter;
            }
            if (logreturn == "a") // 로그인 성공
            {
                int t; // 1 대 1 채팅 목록 수
                int g; // 그룹 채팅 목록 수
                string nick = c.get_nickname(id);
                // 자기 닉네임 수신
                write(clnt_sock, nick.c_str(), strlen(nick.c_str()));
                pthread_mutex_lock(&mutx);    // 뮤텍스 시작
                Client_list(nick, clnt_sock); // 아이디 클라이언트 번호 저장
                pthread_mutex_unlock(&mutx);  // 뮤텍스 종료

                string u_id = c.get_uid(id); // 로그인한 유저의 고유번호를 얻기 위함.
                c.login_connect(id);
                count = 0;
                while (1)
                {
                    cout << "while문 시작\n";
                    string ti;
                    string y = "ONE";
                    t = c.check_chatcount(nick, y);
                    ti = to_string(t);
                    strcpy(sendmsg, ti.c_str());
                    sendmsg[strlen(ti.c_str())] = 0;
                    write(clnt_sock, sendmsg, strlen(sendmsg)); // 1 대 1 채팅 목록 수 발신

                    string gi;
                    y = "GROUP";
                    g = c.check_chatcount(nick, y);
                    gi = to_string(g);
                    strcpy(sendmsg, gi.c_str());
                    sendmsg[strlen(gi.c_str())] = 0;
                    write(clnt_sock, sendmsg, strlen(sendmsg)); // 그룹 채팅 목록 수 발신
                    while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1) // 로그인 성공 후 선택지 수신
                        error_handling("read() error");
                    recv_msg[str_len] = 0;
                    std::string num(recv_msg);
                    if (num == "1") // 친구 요청 수신
                    {
                        while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1) // f_id 수신
                            error_handling("read() error");
                        recv_msg[str_len] = 0;
                        std::string f_id(recv_msg);
                        std::string answer = c.request_friend(u_id, f_id);
                        strcpy(sendmsg, answer.c_str());
                        sendmsg[strlen(answer.c_str())] = 0;
                        write(clnt_sock, sendmsg, strlen(sendmsg)); // 친구 요청하기 함수의 리턴 값 발신
                    }
                    else if (num == "2") // 친구 요청란 확인 후, 친구 요청 수락 및 거절
                    {
                        int a = 0;
                        a = requestlist(u_id, clnt_sock); // 친구요청란을 보내는 함수
                        if (a)
                        {
                            while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1) // 친구 요청을 수락한 유저 고유번호 수신
                                error_handling("read() error");
                            recv_msg[str_len] = 0;
                            std::string f_id(recv_msg);
                            while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1) // 친구 요청 수락 및 거절 수신
                                error_handling("read() error");
                            recv_msg[str_len] = 0;
                            std::string q(recv_msg);
                            if (q == "1")
                            {
                                std::string answer = c.accept_friend(u_id, f_id);
                                strcpy(sendmsg, answer.c_str());
                                sendmsg[strlen(answer.c_str())] = 0;
                            }
                            else if (q == "2")
                            {
                                std::string answer = c.reject_friend(u_id, f_id);
                                strcpy(sendmsg, answer.c_str());
                                sendmsg[strlen(answer.c_str())] = 0;
                            }
                            else
                            {
                                cout << "error" << endl;
                            }
                            write(clnt_sock, sendmsg, strlen(sendmsg)); // 친구 수락 함수의 리턴 값 발신
                        }
                    }
                    else if (num == "3") // 친구의 접속여부 확인하기
                    {
                        int a = 0;
                        a = friendlist(u_id, clnt_sock); // 친구 목록을 보내는 함수
                        if (a)
                        {
                            while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1) // 접속 상태를 확인할 친구 고유번호 수신
                                error_handling("read() error");
                            recv_msg[str_len] = 0;
                            std::string f_id(recv_msg);
                            std::string answer = c.check_connect(f_id);
                            strcpy(sendmsg, answer.c_str());
                            sendmsg[strlen(answer.c_str())] = 0;
                            write(clnt_sock, sendmsg, strlen(sendmsg)); // 친구 접속 여부 확인 함수의 리턴 값 발신
                        }
                    }
                    else if (num == "4") // 1 대 1 채팅 요청하기
                    {
                        string type = "ONE";
                        while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1) // 채팅할 닉네임 수신
                            error_handling("read() error");
                        recv_msg[str_len] = 0;
                        std::string want(recv_msg);
                        cl_sock = clnt(want);
                        c.insert_chat(nick, clnt_sock, want, cl_sock, type);
                        while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
                        {
                            string temp(msg);
                            if (temp == "q\n" || temp == "Q\n")
                            {
                                break;
                            }
                            else
                            {
                                send_msg1(msg, str_len, clnt_sock, cl_sock);
                            }
                        }
                    }
                    else if (num == "5") // 1 대 1 채팅 확인 및 수락, 거절
                    {
                        int cl_n;
                        string type = "ONE";
                        chatlist(nick, clnt_sock, type);
                        while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1) // 친구 요청 수락 및 거절 수신
                            error_handling("read() error");
                        recv_msg[str_len] = 0;
                        std::string choose(recv_msg);
                        cout << choose << endl;
                        if (choose == "Q")
                        {
                            c.check_onechat(choose, nick, clnt_sock, type);
                        }
                        else
                        {
                            c.check_onechat(choose, nick, clnt_sock, type);
                            while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
                            {
                                string temp(msg);
                                if (temp == "q\n" || temp == "Q\n")
                                {
                                    break;
                                }
                                else
                                {
                                    send_msg(msg, str_len, choose, nick);
                                }
                            }
                        }
                    }
                    else if (num == "6") // 단체 채팅 요청하기
                    {
                        string type = "GROUP";
                        while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1) // 단체 채팅방에 초대할 유저 수 수신
                            error_handling("read() error");
                        cout << "recv_msg: " << recv_msg << endl;
                        recv_msg[str_len] = 0;
                        std::string gt(recv_msg);
                        int gc = stoi(gt);
                        gc += 1;
                        int group_cl[gc];
                        group_cl[0] = clnt_sock;

                        for (int p = 1; p < gc; p++) //
                        {
                            while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1) // 채팅할 닉네임 수신
                                error_handling("read() error");
                            recv_msg[str_len] = 0;
                            std::string want(recv_msg);
                            cl_sock = clnt(want);
                            group_cl[p] = cl_sock;
                            c.insert_chat(nick, clnt_sock, want, cl_sock, type);
                        }
                        while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
                        {
                            string temp(msg);
                            if (temp == "q\n" || temp == "Q\n")
                            {
                                sleep(0);
                                break;
                            }
                            else
                            {
                                send_groupmsg1(msg, str_len, group_cl, gc);
                            }
                        }
                    }
                    else if (num == "7") // 단체 채팅 확인 및 수락, 거절 (신청 받은 사람들 들어가는 곳)
                    {
                        int cl_n;
                        string type = "GROUP";
                        chatlist(nick, clnt_sock, type);
                        while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1) // 단체 채팅을 원하는 유저의 닉네임 수신
                            error_handling("read() error");
                        recv_msg[str_len] = 0;
                        std::string choose(recv_msg);
                        cout << choose << endl;
                        if (choose == "Q")
                        {
                            c.check_onechat(choose, nick, clnt_sock, type);
                        }
                        else
                        {
                            c.check_onechat(choose, nick, clnt_sock, type);
                            while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
                            {
                                string temp(msg);
                                if (temp == "q\n" || temp == "Q\n")
                                {
                                    break;
                                }
                                else
                                {
                                    send_groupmsg(msg, str_len, choose, nick);
                                }
                            }
                        }
                    }
                    else if (num == "8") // 랜덤 채팅방
                    {
                        while (1)
                        {
                            while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1)
                                error_handling("read() error");
                            recv_msg[str_len] = 0;
                            string r_chat(recv_msg);
                            if (r_chat == "q" || r_chat == "Q")
                            {
                                break;
                            }
                            else
                            {
                                ranchat.push_back(clnt_sock);
                                if (r_count == 2)
                                {
                                    for (i = 0; i < 2; i++)
                                    {
                                        ranchat.erase(ranchat.begin());
                                    }
                                    r_count = 0;
                                }
                                r_count++;
                                break;
                            }
                        }
                        while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
                        {
                            string temp(msg);
                            if (temp == "q\n" || temp == "Q\n")
                            {
                                break;
                            }
                            else
                            {
                                rand_msg(msg, str_len);
                            }
                        }
                    }
                    else
                    {
                        c.logout_connect(id);
                        c.logout_chatlist(nick);
                        break;
                    }
                }
            }
            else // 로그인 실패
            {
                count++;
                continue;
            }
        }
        // 완성
        else if (choice == "2" || choice == "회원가입")
        {
            while (1)
            {
                std::string u_id;
                std::string u_nickname;
                while (1)
                {
                    // 아이디 수신
                    while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1)
                        error_handling("read() error");
                    recv_msg[str_len] = 0;
                    string id(recv_msg);
                    pthread_mutex_lock(&mutx);
                    std::string check_id = c.check_ID(id);
                    pthread_mutex_unlock(&mutx);
                    // id결과 송신
                    write(clnt_sock, check_id.c_str(), strlen(check_id.c_str()));
                    u_id = id;
                    if (check_id == "a") // 중복 x
                        break;
                }
                // pw 수신
                while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1)
                    error_handling("read() error");
                recv_msg[str_len] = 0;
                std::string pw(recv_msg);
                while (1)
                {
                    // nickname 수신
                    while ((str_len = read(clnt_sock, recv_msg, sizeof(recv_msg))) == -1)
                        error_handling("read() error");
                    recv_msg[str_len] = 0;
                    string nickname(recv_msg);
                    // std::string mail(recv_msg);
                    std::string check_nickname = c.check_nickname(nickname);
                    // check_nickname 송신
                    write(clnt_sock, check_nickname.c_str(), strlen(check_nickname.c_str()));
                    u_nickname = nickname;
                    if (check_nickname == "a")
                        break;
                }
                c.user_info(u_id, pw, u_nickname);
                int UID = c.show_uid();
                string uid = to_string(UID);
                // 고유번호 수신
                write(clnt_sock, uid.c_str(), strlen(uid.c_str()));
                break;
            }
        }
    }

    pthread_mutex_lock(&mutx);
    for (i = 0; i < clnt_cnt; i++) // remove disconnected clinet
    {
        if (clnt_sock == clnt_socks[i])
        {
            while (i++ < clnt_cnt - 1)
                clnt_socks[i] = clnt_socks[i + 1];
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutx);
    close(clnt_sock);
    return NULL;
}

void send_msg(char *msg, int len, string nick, string want) // 신청 받은 사람 (DB에서 가져오기)
{
    chat d("OPERATOR", "1234");
    string type = "ONE";
    int who_clnt = d.take_whoclnt(nick, want, type);
    int whom_clnt = d.take_whomclnt(want, nick);
    int clnt_socks1[2];
    clnt_socks1[0] = who_clnt;
    clnt_socks1[1] = whom_clnt;
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < 2; i++)
        write(clnt_socks1[i], msg, len);
    pthread_mutex_unlock(&mutx);
}

void send_groupmsg(char *msg, int len, string nick, string want) // 신청 받은 사람 (DB에서 가져오기)
{
    chat d("OPERATOR", "1234");
    string type = "GROUP";
    int who_clnt = d.take_whoclnt(nick, want, type); // 신청한 사람의 클라이언트 번호

    // 본인 포함 단체 채팅방으로 초대한 사람들의 클라이언트 번호
    vector<int> whom_clnt;
    whom_clnt = d.takeg_whomclnt(nick, who_clnt);
    int wc = whom_clnt.size();
    cout << "wc: " << wc << endl;
    int clnt_socks1[wc + 1];
    clnt_socks1[0] = who_clnt;
    for (int q = 0; q < wc ; q++)
    {
        clnt_socks1[q + 1] = whom_clnt[q];
    }
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < wc + 1; i++)
        write(clnt_socks1[i], msg, len);
    pthread_mutex_unlock(&mutx);
}

void send_msg1(char *msg, int len, int clnt_sock, int cl_sock) // 신청한 사람(구조체에서 가져오기)
{
    chat d("OPERATOR", "1234");
    int clnt_socks1[2];
    clnt_socks1[0] = clnt_sock;
    clnt_socks1[1] = cl_sock;
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < 2; i++)
        write(clnt_socks1[i], msg, len);
    pthread_mutex_unlock(&mutx);
}

void send_groupmsg1(char *msg, int len, int *group_cl, int p) // 신청한 사람(구조체에서 가져오기)
{
    chat d("OPERATOR", "1234");
    pthread_mutex_lock(&mutx);
    for (int i = 0; i < p; i++)
        write(group_cl[i], msg, len);
    pthread_mutex_unlock(&mutx);
}

void error_handling(std::string msg)
{
    std::cout << msg << errno << std::endl;
    exit(1);
}

int requestlist(string u_id, int clnt_sock)
{
    try
    {
        int k = 0;
        int f_id;
        char sendmsg[BUF_SIZE];
        sql::Driver *driver = sql::mariadb::get_driver_instance();
        sql::SQLString url("jdbc:mariadb://localhost:3306/CHAT");
        sql::Properties properties({{"user", "OPERATOR"}, {"password", "1234"}});
        std::unique_ptr<sql::Connection> conn(driver->connect(url, properties));
        sql::Statement *stmnt = conn->createStatement();
        sql::ResultSet *res = stmnt->executeQuery("select U_ID, NICKNAME from USER where U_ID in (select REQUEST from F_REQUEST where ACCEPT = " + u_id + " and R_CHECK = 'ING')");
        while (res->next())
        {
            ++k;
        }
        string k1 = to_string(k);
        strcpy(sendmsg, k1.c_str());
        sendmsg[strlen(k1.c_str())] = 0;
        write(clnt_sock, sendmsg, strlen(sendmsg)); // 친구 요청란의 갯수를 클라에 보내고, 그 갯수만큼 for문 돌려서 친구요청란 받기 위함.
        if (!k)
        {
            return 0;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error Connecting to MariaDB Platform: " << e.what() << std::endl;
    }
    try
    {
        sql::Driver *driver = sql::mariadb::get_driver_instance();
        sql::SQLString url("jdbc:mariadb://localhost:3306/CHAT");
        sql::Properties properties({{"user", "OPERATOR"}, {"password", "1234"}});
        std::unique_ptr<sql::Connection> conn(driver->connect(url, properties));
        sql::Statement *stmnt1 = conn->createStatement();
        sql::ResultSet *res1 = stmnt1->executeQuery("select U_ID, NICKNAME from USER where U_ID in (select REQUEST from F_REQUEST where ACCEPT = " + u_id + " and R_CHECK = 'ING')");
        int f_id;
        string nickname;
        char sendmsg[BUF_SIZE];
        while (res1->next())
        {
            f_id = res1->getInt(1);
            string f_id1 = to_string(f_id);
            strcpy(sendmsg, f_id1.c_str());
            sendmsg[strlen(f_id1.c_str())] = 0;
            write(clnt_sock, sendmsg, strlen(sendmsg)); // 친구 요청란에 있는 요청한 친구의 고유번호 송신
            sleep(1);

            nickname = res1->getString(2);
            strcpy(sendmsg, nickname.c_str());
            sendmsg[strlen(nickname.c_str())] = 0;
            write(clnt_sock, sendmsg, strlen(sendmsg)); // 친구 목록에 있는 친구의 고유번호 송신
            sleep(1);
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error Connecting to MariaDB Platform: " << e.what() << std::endl;
    }
    return 1;
}

int friendlist(string u_id, int clnt_sock)
{
    try
    {
        int k = 0;
        int f_id;
        char sendmsg[BUF_SIZE];
        sql::Driver *driver = sql::mariadb::get_driver_instance();
        sql::SQLString url("jdbc:mariadb://localhost:3306/CHAT");
        sql::Properties properties({{"user", "OPERATOR"}, {"password", "1234"}});
        std::unique_ptr<sql::Connection> conn(driver->connect(url, properties));
        sql::Statement *stmnt = conn->createStatement();
        sql::ResultSet *res = stmnt->executeQuery("select U_id, NICKNAME from USER natural join F_LIST where U_id in (select F_id from F_LIST where U_id = " + u_id + ") group by U_id");
        while (res->next())
        {
            ++k;
        }
        string k1 = to_string(k);
        int a = strlen(k1.c_str());
        strcpy(sendmsg, k1.c_str());
        sendmsg[strlen(k1.c_str())] = 0;
        write(clnt_sock, sendmsg, strlen(sendmsg)); // 친구의 수를 클라에 보내고, 그 수만큼 조건문 돌려서 친구 목록을 받기 위함.
        if (!k)
        {
            return 0;
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error Connecting to MariaDB Platform: " << e.what() << std::endl;
    }
    try
    {
        sql::Driver *driver = sql::mariadb::get_driver_instance();
        sql::SQLString url("jdbc:mariadb://localhost:3306/CHAT");
        sql::Properties properties({{"user", "OPERATOR"}, {"password", "1234"}});
        std::unique_ptr<sql::Connection> conn(driver->connect(url, properties));
        sql::Statement *stmnt1 = conn->createStatement();
        sql::ResultSet *res1 = stmnt1->executeQuery("select U_id, NICKNAME from USER natural join F_LIST where U_id in (select F_id from F_LIST where U_id = " + u_id + ") group by U_id");
        int f_id;
        string nickname;
        char sendmsg[BUF_SIZE];
        while (res1->next())
        {
            sleep(0);
            f_id = res1->getInt(1);
            string f_id1 = to_string(f_id);
            strcpy(sendmsg, f_id1.c_str());
            sendmsg[strlen(f_id1.c_str())] = 0;
            write(clnt_sock, sendmsg, strlen(sendmsg)); // 친구 목록에 있는 친구의 고유번호 송신
            sleep(0);
            nickname = res1->getString(2);
            strcpy(sendmsg, nickname.c_str());
            sendmsg[strlen(nickname.c_str())] = 0;
            write(clnt_sock, sendmsg, strlen(sendmsg)); // 친구 목록에 있는 친구의 고유번호 송신
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error Connecting to MariaDB Platform: " << e.what() << std::endl;
    }
    return 1;
}

void Client_list(string id, int clnt_sock)
{
    cl.cl_sock = clnt_sock;
    cl.cl_nick = id;
    client_info.push_back(cl);
}

int clnt(string nick)
{
    int cl_sock;
    for (vector<struct Client>::iterator it = client_info.begin(); it != client_info.end(); it++)
    {
        if (it->cl_nick == nick)
            cl_sock = it->cl_sock;
    }
    return cl_sock;
}

void chatlist(string nick, int clnt_sock, string type)
{
    try
    {
        sql::Driver *driver = sql::mariadb::get_driver_instance();
        sql::SQLString url("jdbc:mariadb://localhost:3306/CHAT");
        sql::Properties properties({{"user", "OPERATOR"}, {"password", "1234"}});
        std::unique_ptr<sql::Connection> conn(driver->connect(url, properties));
        sql::Statement *stmnt1 = conn->createStatement();
        sql::ResultSet *res1 = stmnt1->executeQuery("select WHO_NICK, WHO_CLNT from LIST_CHAT where WHOM_NICK = '" + nick + "' and R_CHECK = 'O' and TYPE = '" + type + "'");
        string who_nick;
        int who_clnt;
        char sendmsg[BUF_SIZE];
        while (res1->next())
        {
            sleep(0);
            who_nick = res1->getString(1);
            strcpy(sendmsg, who_nick.c_str());
            sendmsg[strlen(who_nick.c_str())] = 0;
            write(clnt_sock, sendmsg, strlen(sendmsg)); // 친구 목록에 있는 친구의 고유번호 송신
            sleep(0);
            who_clnt = res1->getInt(2);
            string who_clnt1 = to_string(who_clnt);
            strcpy(sendmsg, who_clnt1.c_str());
            sendmsg[strlen(who_clnt1.c_str())] = 0;
            write(clnt_sock, sendmsg, strlen(sendmsg)); // 친구 목록에 있는 친구의 고유번호 송신
        }
    }
    catch (sql::SQLException &e)
    {
        std::cerr << "Error Connecting to MariaDB Platform: " << e.what() << std::endl;
    }
}

void rand_msg(const char *msg, int len) // send to all
{
    int i;
    int a = ranchat[0];
    int b = ranchat[1];
    pthread_mutex_lock(&mutx);
    write(a, msg, len);
    write(b, msg, len);
    pthread_mutex_unlock(&mutx);
}
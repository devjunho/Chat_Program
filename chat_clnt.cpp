#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUF_SIZE 1024
#define NAME_SIZE 20
using namespace std;

void *handle_serv(void *arg, pthread_t snd_thread, pthread_t rcv_thread, void *thread_return);
void error_handling(std::string msg);
void *send_msg(void *arg);
void *recv_msg(void *arg);

char name[NAME_SIZE] = "[DEFAULT]";
char msg[BUF_SIZE];

int main(int argc, char *argv[])
{
    int sock;
    struct sockaddr_in serv_addr;
    pthread_t snd_thread, rcv_thread;
    void *thread_return;
    if (argc != 4)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    handle_serv((void *)&sock, snd_thread, rcv_thread, thread_return);
    close(sock);
    return 0;
}

void *handle_serv(void *arg, pthread_t snd_thread, pthread_t rcv_thread, void *thread_return) // read thread main
{
    int sock = *((int *)arg);
    char msg[BUF_SIZE];
    int str_len;
    while (1)
    {
        int count = 1;
        while (1)
        {
            cout << "1: 로그인\n2: 회원가입\nQ: 종료\n:";

            std::string choice;
            getline(std::cin, choice);
            write(sock, choice.c_str(), strlen(choice.c_str()));

            if (choice == "q" || choice == "Q")
            {
                write(sock, msg, strlen(msg));
                std::cout << "채팅 프로그램을 종료합니다.\n";
                close(sock);
                exit(0);
                break;
            }
            else if (choice == "1" || choice == "로그인")
            {
                // 로그인 유도문
                std::cout << "-----------------------\n"
                          << "         로그인\n"
                          << "-----------------------\n"
                          << "로그인 시도 횟수:" << count << endl;
                cout << " ID: ";
                // id 입력
                std::string id;
                getline(std::cin, id);
                // id 송신
                write(sock, id.c_str(), strlen(id.c_str()));
                // pw 입력
                std::cout << " PW: ";
                std::string pw;
                getline(std::cin, pw);
                // pw 송신
                write(sock, pw.c_str(), strlen(pw.c_str()));
                if (count > 5)
                {
                    // 고유번호 입력
                    cout << " 고유번호: ";
                    std::string u_id;
                    getline(std::cin, u_id);
                    // 고유번호 송신
                    write(sock, u_id.c_str(), strlen(u_id.c_str()));
                }
                // 로그인 결과 수신
                while ((str_len = read(sock, msg, sizeof(msg))) == -1)
                    error_handling("read() error");
                msg[str_len] = 0;
                // 로그인 성공
                std::string login(msg);
                if (login == "a")
                {
                    memset(msg, 0, sizeof(msg));
                    while ((str_len = read(sock, msg, sizeof(msg))) == -1) // 닉네임 받기
                        error_handling("read() error");
                    string nicKname(msg);
                    string nicKname1 = "[" + nicKname + "]";
                    strcpy(name, nicKname1.c_str());
                    count = 1;
                    cout << "로그인 성공\n";
                    while (1)
                    {
                        memset(msg, 0, sizeof(msg));
                        while ((str_len = read(sock, msg, sizeof(msg))) == -1) // 1 대 1 채팅 목록 수 수신
                            error_handling("read() error");
                        msg[str_len] = 0;
                        std::string t(msg);
                        while ((str_len = read(sock, msg, sizeof(msg))) == -1) // 그룹 채팅 목록 수 수신
                            error_handling("read() error");
                        msg[str_len] = 0;
                        std::string g(msg);
                        cout << "1) 친구요청 보내기\n2) 친구요청 확인하기\n3) 친구 접속여부 확인하기\n4) 1 대 1 채팅 요청하기\n";
                        cout << "5) 1 대 1 채팅 요청 '" << t << "'건 존재합니다.\n6) 단체 채팅방 개설하기\n";
                        cout << "7) 단체 채팅 요청 '" << g << "'건 존재합니다.\n8) 랜덤 채팅방\n9) 종료\n";
                        std::string num;
                        getline(std::cin, num);
                        write(sock, num.c_str(), strlen(num.c_str())); // 로그인 성공 후 선택지 발신

                        if (num == "1") // 친구요청 보내기
                        {
                            cout << "친구 요청을 보낼 유저의 고유번호를 입력하시오: ";
                            std::string f_id;
                            getline(std::cin, f_id);
                            write(sock, f_id.c_str(), strlen(f_id.c_str()));       // f_id 발신
                            while ((str_len = read(sock, msg, sizeof(msg))) == -1) // 친구 요청하기 함수의 리턴 값 수신
                                error_handling("read() error");
                            msg[str_len] = 0;
                            std::string answer(msg);
                            if (answer == "a")
                            {
                                cout << "친구 요청을 보냈습니다." << endl;
                            }
                            else if (answer == "b")
                            {
                                cout << "친구 요청 실패하였습니다." << endl;
                            }
                            else
                            {
                                cout << "error" << endl;
                            }
                        }
                        else if (num == "2") // 친구 요청란 확인 후, 친구 요청 수락 및 거절
                        {
                            int k = 0;
                            string fr_id;
                            cout << "친구 요청란입니다." << endl;
                            while ((str_len = read(sock, msg, sizeof(msg))) == -1) // 친구요청란의 친구고유번호 갯수를 받음. 이후에 갯수만큼 조건문을 돌리기 위해서.
                                error_handling("read() error");
                            msg[str_len] = 0;
                            k = atoi(msg);
                            if (k >= 1)
                            {
                                int i = 0;
                                while (i != k)
                                {
                                    str_len = read(sock, msg, sizeof(msg));
                                    if (str_len == -1)
                                        error_handling("read() error");
                                    msg[str_len] = 0;
                                    int j = 0;
                                    j = atoi(msg);
                                    str_len = read(sock, msg, sizeof(msg));
                                    if (str_len == -1)
                                        error_handling("read() error");
                                    msg[str_len] = 0;
                                    string nickname(msg);
                                    cout << "F_ID: " << j << "      NICKNAME: " << nickname << endl;
                                    i++;
                                }
                                cout << "친구 요청에 응답할 유저 고유번호를 입력하시오: ";
                                getline(cin, fr_id);
                                write(sock, fr_id.c_str(), strlen(fr_id.c_str())); // 친구 요청을 수락한 유저 고유번호 발신
                                string q;
                                cout << "1) 수락    2) 거절\n:";
                                getline(cin, q);
                                write(sock, q.c_str(), strlen(q.c_str()));             // 친구 요청 수락 및 거절 발신
                                while ((str_len = read(sock, msg, sizeof(msg))) == -1) // 친구 요청 수락 및 거절 함수 리턴 값 수신
                                    error_handling("read() error");
                                msg[str_len] = 0;
                                std::string answer(msg);
                                if (answer == "a")
                                {
                                    cout << "친구 요청 응답이 완료되었습니다." << endl;
                                }
                                else
                                {
                                    cout << "error" << endl;
                                }
                            }
                            else
                            {
                                cout << "친구 요청이 없습니다." << endl;
                            }
                        }
                        else if (num == "3") // 친구의 접속여부 확인하기
                        {
                            int k = 0;
                            string fr_id;
                            cout << "친구 목록입니다." << endl;
                            while ((str_len = read(sock, msg, sizeof(msg))) == -1) // 친구목록의 친구 수를 받음. 이후에 친구 수만큼 조건문을 돌리기 위해서.
                                error_handling("read() error");
                            msg[str_len] = 0;
                            k = atoi(msg);
                            if (k >= 1)
                            {
                                int i = 0;
                                while (i != k)
                                {
                                    str_len = read(sock, msg, sizeof(msg));
                                    if (str_len == -1)
                                        error_handling("read() error");
                                    msg[str_len] = 0;
                                    int j = 0;
                                    j = atoi(msg);
                                    str_len = read(sock, msg, sizeof(msg));
                                    if (str_len == -1)
                                        error_handling("read() error");
                                    msg[str_len] = 0;
                                    string nickname(msg);
                                    cout << "F_ID: " << j << "      NICKNAME: " << nickname << endl;
                                    i++;
                                }
                                cout << "접속여부를 확인할 친구 고유번호를 입력하시오: ";
                                getline(cin, fr_id);
                                write(sock, fr_id.c_str(), strlen(fr_id.c_str()));     // 접속여부를 확인할 친구의 고유번호 발신
                                while ((str_len = read(sock, msg, sizeof(msg))) == -1) // 접속여부 함수의 리턴 값 수신
                                    error_handling("read() error");
                                msg[str_len] = 0;
                                std::string answer(msg);
                                if (answer == "a")
                                {
                                    cout << "고유번호 " << fr_id << "번 친구는 현재 '로그인' 상태입니다." << endl;
                                }
                                else if (answer == "b")
                                {
                                    cout << "고유번호 " << fr_id << "번 친구는 현재 '로그아웃' 상태입니다." << endl;
                                }
                                else
                                {
                                    cout << "error" << endl;
                                }
                            }
                            else
                            {
                                cout << "접속한 친구가 없습니다." << endl;
                            }
                        }
                        else if (num == "4") // 1 대 1 채팅 요청하기
                        {
                            string want;
                            cout << "1 대 1 채팅 신청할 유저의 닉네임을 입력하시오: " << endl;
                            getline(cin, want);
                            write(sock, want.c_str(), strlen(want.c_str())); // 1 대 1 채팅 신청할 유저의 닉네임 발신
                            cout << "1 대 1 채팅방에 입장하셨습니다.\n잠시만 기다려주세요.\nQ) 나가기\n";
                            pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);
                            pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
                            pthread_join(snd_thread, &thread_return);
                            pthread_detach(rcv_thread);
                            cout << "채팅방에서 나가셨습니다." << endl;
                        }
                        else if (num == "5") // 1 대 1 채팅 확인 및 수락, 거절
                        {
                            string choose;
                            int ti = stoi(t);
                            if (ti >= 1)
                            {
                                int i = 0;
                                while (i != ti)
                                {
                                    str_len = read(sock, msg, sizeof(msg));
                                    if (str_len == -1)
                                        error_handling("read() error");
                                    msg[str_len] = 0;
                                    string nickname(msg);

                                    str_len = read(sock, msg, sizeof(msg));
                                    if (str_len == -1)
                                        error_handling("read() error");
                                    msg[str_len] = 0;
                                    int j = 0;
                                    j = atoi(msg);
                                    cout << "F_ID: " << j << "      NICKNAME: " << nickname << endl;
                                    i++;
                                }
                                cout << "1 대 1 채팅을 원하는 유저의 닉네임을 입력하시오(해당 창을 나가고 싶을 시 'Q'): \n"; // Q 안먹음;
                                getline(cin, choose);
                                write(sock, choose.c_str(), strlen(choose.c_str()));
                                cout << "1 대 1 채팅방에 입장하셨습니다\nQ) 나가기\n";
                                pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);
                                pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
                                pthread_join(snd_thread, &thread_return);
                                pthread_detach(rcv_thread);
                                cout << "채팅방에서 나가셨습니다." << endl;
                            }
                        }
                        else if (num == "6") // 단체 채팅 요청하기
                        {
                            int gc;
                            string gcs;
                            string want;
                            cout << "단체 채팅방에 초대할 유저 수를 입력하시오: " << endl;
                            getline(cin, gcs);
                            write(sock, gcs.c_str(), strlen(gcs.c_str())); // 단체 채팅방에 초대할 유저 수 발신
                            gc = stoi(gcs);

                            for (int p = 0; p < gc; p++)
                            {
                                cout << "단체 채팅 신청할 유저의 닉네임을 입력하시오: " << endl;
                                getline(cin, want);
                                write(sock, want.c_str(), strlen(want.c_str())); // 단체 채팅 신청할 유저의 닉네임 발신
                            }
                            cout << "단체 채팅방에 입장하셨습니다.\n잠시만 기다려주세요.\nQ) 나가기\n";
                            pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);
                            pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
                            pthread_join(snd_thread, &thread_return);
                            pthread_detach(rcv_thread);
                            cout << "채팅방에서 나가셨습니다." << endl;
                        }
                        else if (num == "7") // 단체 채팅 확인 및 수락, 거절
                        {
                            string choose;
                            int gi = stoi(g);
                            if (gi >= 1)
                            {
                                int i = 0;
                                while (i != gi)
                                {
                                    str_len = read(sock, msg, sizeof(msg));
                                    if (str_len == -1)
                                        error_handling("read() error");
                                    msg[str_len] = 0;
                                    string nickname(msg);

                                    str_len = read(sock, msg, sizeof(msg));
                                    if (str_len == -1)
                                        error_handling("read() error");
                                    msg[str_len] = 0;
                                    int j = 0;
                                    j = atoi(msg);
                                    cout << "F_ID: " << j << "      NICKNAME: " << nickname << endl;
                                    i++;
                                }
                                cout << "단체 채팅을 원하는 유저의 닉네임을 입력하시오(해당 창을 나가고 싶을 시 'Q'): \n"; // Q 안먹음;
                                getline(cin, choose);
                                write(sock, choose.c_str(), strlen(choose.c_str())); // 단체 채팅을 원하는 유저의 닉네임 발신
                                
                                cout << "단체 채팅방에 입장하셨습니다\nQ) 나가기\n";
                                pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);
                                pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
                                pthread_join(snd_thread, &thread_return);
                                pthread_detach(rcv_thread);
                                cout << "채팅방에서 나가셨습니다." << endl;
                            }
                        }
                        else if (num == "8")
                        {
                            while (1)
                            {
                                cout << "랜덤 채팅방 입장: y (나가기 q)\n";
                                string r_chat;
                                getline(cin, r_chat);
                                if (r_chat == "q" || r_chat == "Q")
                                {
                                    cout << "랜덤 채팅 프로그램을 종료합니다.\n";
                                    break;
                                }
                                // 입장 송신
                                write(sock, r_chat.c_str(), sizeof(r_chat.c_str()));
                                cout << "대기중....\n";

                                pthread_create(&snd_thread, NULL, send_msg, (void *)&sock);
                                pthread_create(&rcv_thread, NULL, recv_msg, (void *)&sock);
                                pthread_join(snd_thread, &thread_return);
                                pthread_detach(rcv_thread);
                                break;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                }
                else if (login == "c")
                {
                    std::cout << "로그인 실패: 아이디가 존재하지 않습니다.\n";
                    count++;
                    continue;
                }
                else if (login == "b")
                {
                    std::cout << "로그인 실패: 비밀번호가 틀립니다.\n";
                    count++;
                    continue;
                }
                else // return = "d"
                {   cout << login << endl;
                    std::cout << "로그인 오류\n";
                    count++;
                    continue;
                }
            }
            else if (choice == "2" || choice == "회원가입")
            {
                while (1)
                {
                    std::string id;
                    while (1)
                    {
                        // 아이디 입력 유도문
                        std::cout << "------------------------\n"
                                  << "ID를 입력하시오.\n";
                        // 아이디 입력
                        getline(std::cin, id);
                        // 아이디 송신
                        write(sock, id.c_str(), strlen(id.c_str()));
                        // id 결과 수신
                        while ((str_len = read(sock, msg, sizeof(msg))) == -1)
                            error_handling("read() error");
                        msg[str_len] = 0;
                        std::string id_check(msg);
                        if (id_check == "a")
                        {
                            break;
                        }
                        else
                        {
                            cout << "다시 입력하세요." << endl;
                            continue;
                        }
                    }
                    // 비밀입력 유도문
                    std::cout << "------------------------\n"
                              << "비밀번호를 입력하시오.\n";
                    std::string pw;
                    // pw 입력
                    getline(std::cin, pw);
                    // pw 송신
                    write(sock, pw.c_str(), strlen(pw.c_str()));
                    while (1)
                    {
                        // nickname 유도문
                        std::cout << "------------------------\n"
                                  << "닉네임을 입력하시오.\n";
                        std::string nickname;
                        // nickname 입력
                        getline(std::cin, nickname);
                        // nickname 송신
                        write(sock, nickname.c_str(), strlen(nickname.c_str()));
                        // mail 수신
                        while ((str_len = read(sock, msg, sizeof(msg))) == -1)
                            error_handling("read() error");
                        msg[str_len] = 0;
                        std::string nickname_check(msg);
                        if (nickname_check == "a")
                            break;
                        else
                            continue;
                    }
                    std::cout << "회원가입 성공!" << std::endl;
                    while ((str_len = read(sock, msg, sizeof(msg))) == -1)
                        error_handling("read() error");
                    msg[str_len] = 0;
                    cout << id << "님의 고유번호는 " << msg << " 입니다\n";
                    count = 1;
                    break;
                }
            }
            else
            {
                std::cout << "잘못된 입력입니다.\n";
                continue;
            }
        }
    }
    return NULL;
}

void error_handling(std::string msg)
{
    std::cout << msg << errno << std::endl;
    exit(1);
}

void *send_msg(void *arg)
{
    int sock = *((int *)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];
    while (1)
    {
        fgets(msg, BUF_SIZE, stdin);
        if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
        {
            write(sock, msg, strlen(name_msg));
            break;
        }
        else
        {
            sprintf(name_msg, "%s %s", name, msg);
            write(sock, name_msg, strlen(name_msg));
        }
    }
    return NULL;
}

void *recv_msg(void *arg)
{
    int sock = *((int *)arg);
    char name_msg[NAME_SIZE + BUF_SIZE];
    int str_len;
    while (1)
    {
        str_len = read(sock, name_msg, NAME_SIZE + BUF_SIZE - 1);
        if (str_len == -1)
            return (void *)-1;
        name_msg[str_len] = 0;
        fputs(name_msg, stdout);
        std::string m(name_msg);
        if (m == "q" || m == "Q")
        {
            cout << "끝\n";
            break;
        }
    }
    return NULL;
}
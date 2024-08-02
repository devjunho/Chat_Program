#include <iostream>
#include <mariadb/conncpp.hpp>
#include <string>
#include <sstream>
#include <vector>

using namespace std;

class DB
{
protected:
    string m_id;
    string m_pw;
    sql::Connection *m_connection;

public:
    DB() {}

    ~DB()
    {
        m_connection->close();
    }

    void account(const string &id, const string &pw)
    {
        this->m_id = id;
        this->m_pw = pw;
    }

    void connect()
    {
        try
        {
            sql::Driver *driver = sql::mariadb::get_driver_instance();
            sql::SQLString url = "jdbc:mariadb://localhost:3306/CHAT";
            sql::Properties properties({{"user", m_id}, {"password", m_pw}});
            m_connection = driver->connect(url, properties);
        }
        catch (sql::SQLException &e)
        {
            cerr << "DB 접속실패: " << e.what() << '\n';
        }
    }
};

class chat : public DB
{
protected:
public:
    chat(){};

    chat(std::string id, std::string pw)
    {
        m_id = id;
        m_pw = pw;
        connect();
    }

    string LOGIN(std::string id, std::string pw, int U_id = 0)
    {
        if (!U_id)
        {
            try
            {
                sql::PreparedStatement *stmnt1 = m_connection->prepareStatement("SELECT ID FROM USER WHERE ID = ?");
                stmnt1->setString(1, id);
                sql::ResultSet *resid = stmnt1->executeQuery();
                if (resid->next())
                {
                    sql::PreparedStatement *stmnt2 = m_connection->prepareStatement("SELECT PW FROM USER WHERE ID = ? AND PW = ?");
                    stmnt2->setString(1, id);
                    stmnt2->setString(2, pw);
                    sql::ResultSet *respw = stmnt2->executeQuery();
                    if (respw->next())
                        return "a"; // 0 아이디 비번 일치
                    else
                        return "b"; // 2 비번 불일치
                }
                else
                    return "c"; // 1 아이디 없음
            }
            catch (sql::SQLException &e)
            {
                std::cerr << "Error selecting USER: " << e.what() << std::endl;
            }
        }
        else
        {
            sql::PreparedStatement *stmnt1 = m_connection->prepareStatement("SELECT ID FROM USER WHERE ID = ?");
            stmnt1->setString(1, id);
            sql::ResultSet *resid = stmnt1->executeQuery();
            if (resid->next())
            {
                sql::PreparedStatement *stmnt2 = m_connection->prepareStatement("SELECT PW, U_id FROM USER WHERE ID = ? AND PW = ? AND U_id = ?");
                stmnt2->setString(1, id);
                stmnt2->setString(2, pw);
                stmnt2->setInt(3, U_id);
                sql::ResultSet *respw = stmnt2->executeQuery();
                if (respw->next())
                    return "a"; // 0 아이디 비번  고유번호 일치
                else
                    return "b"; // 2 비번 불일치
            }
            else
                return "c"; // 1 아이디 없음
        }
        return "d";
    }

    void login_connect(std::string id)
    {
        try
        {
            sql::Statement *stmnt(m_connection->createStatement());
            sql::ResultSet *res = stmnt->executeQuery("update USER set CONNECT = 1 where ID = '" + id + "'");
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error login connect: " << e.what() << std::endl;
        }
    }

    void logout_connect(std::string id)
    {
        try
        {
            sql::Statement *stmnt(m_connection->createStatement());
            sql::ResultSet *res = stmnt->executeQuery("update USER set CONNECT = 0 where ID = '" + id + "'");
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error logout connect: " << e.what() << std::endl;
        }
    }

    void logout_chatlist(std::string nickname)
    {
        try
        {
            sql::Statement *stmnt(m_connection->createStatement());
            sql::ResultSet *res = stmnt->executeQuery("update LIST_CHAT set R_CHECK = 'X' where WHO_NICK = '" + nickname + "'");
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error logout connect: " << e.what() << std::endl;
        }
    }

    void user_info(std::string id, std::string pw, std::string nickname)
    {
        try
        {
            sql::PreparedStatement *ui = m_connection->prepareStatement("INSERT INTO USER (ID, PW, NICKNAME) VALUES (? , ? , ?)");
            ui->setString(1, id);
            ui->setString(2, pw);
            ui->setString(3, nickname);
            sql::ResultSet *res = ui->executeQuery();
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting USER: " << e.what() << std::endl;
        }
    }

    // 아이디 중복 검사
    std::string check_ID(std::string ID)
    {
        try
        {
            // 아이디 중복 확인
            sql::Statement *stmnt2(m_connection->createStatement());
            sql::ResultSet *res = stmnt2->executeQuery("select ID from USER where ID =  '" + ID + "'");
            while (res->next())
            {
                return "b"; // 아이디 중복
            }
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting USER: " << e.what() << std::endl;
            return "c"; // 오류
        }
        return "a";
    }

    // nickname 중복
    std::string check_nickname(std::string nickname)
    {
        try
        { // 이메일 중복 확인
            sql::Statement *stmnt2 = m_connection->createStatement();
            sql::ResultSet *res = stmnt2->executeQuery("select * from USER where NICKNAME = '" + nickname + "'");
            while (res->next())
            {

                if (nickname == res->getString(6))
                {
                    return "b"; // 닉네임 중복 시
                }
            }
            // 실패시 오류 메세지 반환
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting USER: " << e.what() << std::endl;
            return "c";
        }
        return "a";
    }

    std::string get_uid(string id) // 유저의 아이디를 매개변수로 받은 후, 유저의 고유번호 얻어오기
    {
        try
        {
            string u_id;
            sql::Statement *stmnt = m_connection->createStatement();
            sql::ResultSet *res = stmnt->executeQuery("select U_ID from USER where ID = '" + id + "'");
            while (res->next())
            {
                u_id = res->getString(1);
            }
            return u_id;
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting USER: " << e.what() << std::endl;
            return "a";
        }
    }

    std::string request_friend(string u_id, string f_id)
    {
        try
        {
            sql::Statement *stmnt = m_connection->createStatement();
            sql::ResultSet *res = stmnt->executeQuery("insert into F_REQUEST values ('" + u_id + "', '" + f_id + "', DEFAULT)");
            return "a";
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error insert into F_REQUEST: " << e.what() << std::endl;
            return "b";
        }
    }

    std::string accept_friend(string u_id, string f_id)
    {
        try
        {
            sql::Statement *stmnt = m_connection->createStatement();
            sql::ResultSet *res = stmnt->executeQuery("update F_REQUEST set R_CHECK = 'O' where REQUEST = '" + f_id + "' and ACCEPT = '" + u_id + "'");

            sql::Statement *stmnt1 = m_connection->createStatement();
            sql::ResultSet *res1 = stmnt1->executeQuery("insert into F_LIST values ('" + u_id + "', '" + f_id + "'), ('" + f_id + "', '" + u_id + "')");
            return "a";
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting tasks: " << e.what() << std::endl;
            return "b";
        }
    }

    std::string reject_friend(string u_id, string f_id)
    {
        try
        {
            sql::Statement *stmnt = m_connection->createStatement();
            sql::ResultSet *res = stmnt->executeQuery("update F_REQUEST set R_CHECK = 'X' where REQUEST = '" + f_id + "' and ACCEPT = '" + u_id + "'");
            return "a";
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting tasks: " << e.what() << std::endl;
            return "b";
        }
    }

    std::string check_connect(string f_id)
    {
        try
        {
            int connect;
            sql::Statement *stmnt = m_connection->createStatement();
            sql::ResultSet *res = stmnt->executeQuery("select CONNECT from USER where U_ID = '" + f_id + "'");
            while (res->next())
            {
                connect = res->getInt(1);
            }
            if (connect == 1)
            {
                return "a";
            }
            else
            {
                return "b";
            }
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting tasks: " << e.what() << std::endl;
            return "c";
        }
    }

    int show_uid()
    {
        try
        {
            int u_id;
            sql::PreparedStatement *show = m_connection->prepareStatement("SELECT MAX(U_ID) FROM USER");
            sql::ResultSet *res = show->executeQuery();
            while (res->next())
            {
                u_id = res->getInt(1);
            }
            return u_id;
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting USER: " << e.what() << std::endl;
            return 0;
        }
    }

    std::string get_nickname(string id) // 유저의 아이디를 매개변수로 받은 후, 유저의 고유번호 얻어오기
    {
        try
        {
            string nickname;
            sql::Statement *stmnt = m_connection->createStatement();
            sql::ResultSet *res = stmnt->executeQuery("select NICKNAME from USER where ID = '" + id + "'");
            while (res->next())
            {
                nickname = res->getString(1);
            }
            return nickname;
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting USER: " << e.what() << std::endl;
            return "a";
        }
    }

    void insert_chat(string who_nickname, int who_clnt, string whom_nickname, int whom_clnt, string type)
    {
        try
        {
            sql::PreparedStatement *stmnt = m_connection->prepareStatement("INSERT INTO LIST_CHAT VALUES (?, ?, ?, ?, 'O', ?)");
            stmnt->setString(1, who_nickname);
            stmnt->setInt(2, who_clnt);
            stmnt->setString(3, whom_nickname);
            stmnt->setInt(4, whom_clnt);
            stmnt->setString(5, type);
            sql::ResultSet *res = stmnt->executeQuery();
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting USER: " << e.what() << std::endl;
        }
    }

    int check_chatcount(string nickname, string type) // 1 대 1 채팅 목록 수 알아보기 위함
    {
        try
        {
            int t = 0;
            sql::PreparedStatement *stmnt = m_connection->prepareStatement("SELECT WHO_NICK, WHO_CLNT FROM LIST_CHAT WHERE WHOM_NICK = ? AND R_CHECK = 'O' AND TYPE = ?");
            stmnt->setString(1, nickname);
            stmnt->setString(2, type);
            sql::ResultSet *res = stmnt->executeQuery();
            while (res->next())
            {
                ++t;
            }
            return t;
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting USER: " << e.what() << std::endl;
            return -1;
        }
    }

    int take_whoclnt(string nickname, string whom, string type)
    {
        try
        {
            int who_clnt;
            sql::PreparedStatement *stmnt = m_connection->prepareStatement("SELECT WHO_CLNT FROM LIST_CHAT WHERE WHO_NICK = ? AND WHOM_NICK = ? AND TYPE = ?");
            stmnt->setString(1, nickname);
            stmnt->setString(2, whom);
            stmnt->setString(3, type);
            sql::ResultSet *res = stmnt->executeQuery();
            while (res->next())
            {
                who_clnt = res->getInt(1);
            }
            return who_clnt;
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting USER: " << e.what() << std::endl;
            return -1;
        }
    }

    int take_whomclnt(string nickname, string who)
    {
        try
        {
            int whom_clnt;
            sql::PreparedStatement *stmnt = m_connection->prepareStatement("SELECT WHOM_CLNT FROM LIST_CHAT WHERE WHOM_NICK = ? AND WHO_NICK = ? AND TYPE = 'ONE'");
            stmnt->setString(1, nickname);
            stmnt->setString(2, who);
            sql::ResultSet *res = stmnt->executeQuery();
            while (res->next())
            {
                whom_clnt = res->getInt(1);
            }
            return whom_clnt;
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting USER: " << e.what() << std::endl;
            return -1;
        }
    }

    vector<int> takeg_whomclnt(string who_nick, int who_clnt)
    {
        vector<int> whom_clnt;
        try
        {
            sql::PreparedStatement *stmnt = m_connection->prepareStatement("SELECT WHOM_CLNT FROM LIST_CHAT WHERE WHO_NICK = ? AND WHO_CLNT = ? AND TYPE = 'GROUP'");
            stmnt->setString(1, who_nick);
            stmnt->setInt(2, who_clnt);
            sql::ResultSet *res = stmnt->executeQuery();
            while (res->next())
            {
                whom_clnt.push_back(res->getInt(1));
            }
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting USER: " << e.what() << std::endl;
        }
        return whom_clnt;
    }

    void check_onechat(string who_nick, string whom_nick, int whom_clnt, string type)
    {
        try
        {
            sql::PreparedStatement *stmnt = m_connection->prepareStatement("UPDATE LIST_CHAT SET R_CHECK = 'X' WHERE WHO_NICK = ? AND WHOM_NICK = ? AND WHOM_CLNT = ? AND TYPE = ?");
            stmnt->setString(1, who_nick);
            stmnt->setString(2, whom_nick);
            stmnt->setInt(3, whom_clnt);
            stmnt->setString(4, type);
            sql::ResultSet *res = stmnt->executeQuery();
        }
        catch (sql::SQLException &e)
        {
            std::cerr << "Error selecting USER: " << e.what() << std::endl;
        }
    }
};
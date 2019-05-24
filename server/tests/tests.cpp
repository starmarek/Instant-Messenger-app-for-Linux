#include "../src/server_secondary_functions.cpp"
#include "../src/user.cpp"
#include "../bin/user.hpp"

#include <gtest/gtest.h>


struct user_classTests : testing::Test {
    
    std::vector<user> v;
    user u = {5, (char*)"john"};

    void SetUp() {

        std::cout.setstate(std::ios_base::failbit);
        v.emplace_back(user(8, (char*)"gustavo"));
        v.emplace_back(user(5, (char*)"noname"));
        v.emplace_back(user(20, (char*)"sebastian"));
        v.emplace_back(user(9, (char*)"caroline"));
        v.emplace_back(user(13, (char*)"bohdan"));


        u.actuallySwitched = 8;
        u.authotizatingCon = 20;
        u.connectedUsers.emplace_back(user(8, (char*)"gustavo"));
        u.connectedUsers.emplace_back(user(9, (char*)"caroline"));

        (*v.begin()).connectedUsers.emplace_back(user(5, (char*)"john"));
    }

    void TearDown() {}
};


struct serverTests : user_classTests { };


//////  USER ///////

TEST_F(user_classTests, c_tor) {

    EXPECT_STREQ(u.username , "john");
    EXPECT_EQ(u.socket , 5);
}


TEST_F(user_classTests, showSwitched) {
     
    char *name = u.showSwitched(v);

    ASSERT_STRNE(name , "Err");
}


TEST_F(user_classTests, showConnected) {
     
    int count = u.showConnectedUsers();

    ASSERT_EQ(count , 2);
}


TEST_F(user_classTests, findConnected) {
     
    int sock = u.findConected((char *)"caroline");

    ASSERT_EQ(sock , 9);
}


TEST_F(user_classTests, disconnect) {
     
    int result = u.disconnect((char *)"caroline");

    ASSERT_EQ(result , 0);
}


//////  SERVER ITSELF ///////

TEST_F(serverTests, pushMessFurth) {
     
    int result = pushMessFurther((char *)"Test right there!", u);

    ASSERT_EQ(result , 0);
}


TEST_F(serverTests, switchUsr) {
    
    char command[] = "switch gustavo";
    int result = switchUser(command, u);

    ASSERT_EQ(result , 0);
}


TEST_F(serverTests, discUsrs) {
    
    char command[] = "disconnect gustavo";

    int result = disconnectUsers(command, v, u);

    ASSERT_EQ(result , 0);
}


TEST_F(serverTests, finConnect) {
    
    int result = finishConnecting((char *)"yes", v, u);

    ASSERT_EQ(result , 0);
}


TEST_F(serverTests, findSockName) {
    
    int result = findSocketByName(v, (char *)"caroline");

    ASSERT_EQ(result , 9);
}


TEST_F(serverTests, setAutho) {
    
    setAuthorization(u, v, 13);

    ASSERT_EQ((*(v.end()-1)).authotizatingCon , 5);
}


TEST_F(serverTests, connUsrs) {

    char command[] = "connect bohdan";
    
    int result = connectUsers(command, v, u);

    ASSERT_EQ(result , 0);
}


TEST_F(serverTests, discAl) {

    disconnectAll((char *)"john", u.connectedUsers, v);

    ASSERT_EQ((*v.begin()).connectedUsers.size() , 0);
}


TEST_F(serverTests, closeConn) {

    std::vector<user>::iterator iter = v.end() - 2;

    std::vector<user>::iterator it = closeConnection(v, iter);

    ASSERT_EQ(it->socket , 13);
}


TEST_F(serverTests, findSockDesc) {

    char *name = findSocketByDescriptor(v, 8);

    ASSERT_STREQ(name , "gustavo");
}


TEST_F(serverTests, setUsrNam) {

    char command[] = "login gustavo";

    int result = setUsername(command, *(v.begin() + 1), v);

    ASSERT_EQ(result , -3);
}
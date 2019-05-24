#include "../src/client_secondary_functions.cpp"
#include <gtest/gtest.h>


//remember to include "foo.txt" in your working directory!
struct queueTest : testing::Test {

    char *fileName2;
    std::vector<char*> v;

    void SetUp() {
        std::cout.setstate(std::ios_base::failbit);
        fileName2 = new char[8];
        strcpy(fileName2, "foo.txt");
        v.push_back(fileName2);
    }

    void TearDown() {
        delete[] fileName2;
    }
};


TEST_F(queueTest, removeFile) {
  
    char command[] = "queue-remove foo.txtx"; //last x in the .txt format imitates "enter key" press 
   
    int result = removeFileQueue(v, command);

    ASSERT_EQ(result , 0);
}


TEST_F(queueTest, addFile) {

    char command[] = "queue-add foo.txtx";

    int result = queueFile(v, command);

    EXPECT_EQ(result , 0);
}


TEST_F(queueTest, showFiles) {

    int result = showQueue(v);

    EXPECT_EQ(result , 0);
}


TEST_F(queueTest, clearFiles) {
  
    int result = killQueue(v);

    EXPECT_EQ(result , 1);
}
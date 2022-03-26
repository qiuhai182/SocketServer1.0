/*
 * @Author: your name
 * @Date: 2022-01-07 20:53:10
 * @LastEditTime: 2022-02-07 00:32:13
 * @LastEditors: Please set LastEditors
 * @Description: 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * @FilePath: /oxc/qReader/scripts/init/init.h
 */
#pragma once

#include <gflags/gflags.h>
#include <arpa/inet.h>
#include <unordered_map>
#include <butil/logging.h>
#include <sys/io.h>
#include "database/tableInfo/UserInfo.hpp"
#include "database/tableInfo/BookBaseInfo.hpp"
#include "database/tableInfo/BookSearchInfo.hpp"
#include "database/tableInfo/BookGradeInfo.hpp"
#include "database/tableInfo/BookShelfInfo.hpp"
#include "database/tableInfo/SightInfo.hpp"
#include "database/tableInfo/ReadIntervalInfo.hpp"
#include "database/tableInfo/BookCommentHitInfo.hpp"
#include "database/tableInfo/BookCommentHitCountInfo.hpp"
#include "database/tableInfo/PageCommentInfo.hpp"
#include "database/tableInfo/PageCommentHitInfo.hpp"
#include "database/tableInfo/BookDownloadCount.hpp"

using namespace std;
using namespace ormpp;

namespace Init 
{
   

  void createSql()
  {

    // try{
    //     BookBaseInfo baseBook(false);
    //     usleep(1000);
    // }catch(const char *error){
    //     cout<<"Error: "<<error<<endl;
    // }
    
    // try{
    //     UserInfo userInfo(false);
    //     usleep(1000);
    // }catch(const char *error){
    //     cout<<"Error: "<<error<<endl;
    // }

    // try{
    //     UserShelfInfo userShelfInfo(false);
    //     usleep(1000);
    // }catch(const char *error){
    //     cout<<"Error: "<<error<<endl;
    // }

    // try{
    //     BookGradeInfo gradeBook(false);
    //     usleep(1000);
    // }catch(const char *error){
    //     cout<<"Error: "<<error<<endl;
    // }
    
    // try{
    //     BookCommentHitInfo gradeBook(false);
    //     usleep(1000);
    // }catch(const char *error){
    //     cout<<"Error: "<<error<<endl;
    // }

    // try{
    //     BookCommentHitCountInfo hitCount(false);
    //     usleep(1000);
    // }catch(const char *error){
    //     cout<<"Error: "<<error<<endl;
    // }

    // try{
    //     BookSearchInfo searchInfo(false);
    //     usleep(1000);
    // }catch(const char *error){
    //     cout<<"Error: "<<error<<endl;
    // }

    // try{
    //     SightInfo searchInfo(false);
    //     usleep(1000);
    // }catch(const char *error){
    //     cout<<"Error: "<<error<<endl;
    // }

    // try{
    //     ReadIntervalInfo intervalInfo(false);
    //     usleep(1000);
    // }catch(const char *error){
    //     cout<<"Error: "<<error<<endl;
    // }

    // try{
    //     PageCommentInfo pagecommentInfo(false);
    //     usleep(1000);
    // }catch(const char *error){
    //     cout<<"Error: "<<error<<endl;
    // }

    // try{
    //     PageCommentHitInfo pagecommentHitInfo(false);
    //     usleep(1000);
    // }catch(const char *error){
    //     cout<<"Error: "<<error<<endl;
    // }

    // try{
    //     BookDownloadCount bookDownloadCount(false);
    //     usleep(1000);
    // }catch(const char *error){
    //     cout<<"Error: "<<error<<endl;
    // }

  }
  
}


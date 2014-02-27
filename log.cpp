/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  <copyright holder> <email>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */

#include "log.h"
#include <pthread.h>
#include <iostream>
Log* sLog = NULL;
pthread_mutex_t Log_Mutex;



Log::Log()
{
    pthread_mutex_init(&Log_Mutex,NULL);
    sLog = this;
    
    
}
void Log::error(std::string msg)
{
    outStr(std::string("\033[1m\033[31m")+"-ERROR- \033[0m\033[31m"+msg+"\033[0m");
}
void Log::info(std::string msg)
{
    outStr(std::string("\033[1m\033[37m")+"-INFO - \033[0m\033[37m"+msg+"\033[0m");
}

void Log::warn(std::string msg)
{
    outStr(std::string("\033[1m\033[33m")+"-WARN - \033[0m\033[33m"+msg+"\033[0m");
}
void Log::debug(std::string msg)
{
    outStr(std::string("\033[1m\033[34m")+"-WARN - \033[0m\033[34m"+msg+"\033[0m");
}


void Log::outStr(std::string msg)
{
    pthread_mutex_lock(&Log_Mutex);
    std::cout << msg << std::endl;
    
    pthread_mutex_unlock(&Log_Mutex);
}

Log::~Log()
{
    pthread_mutex_destroy(&Log_Mutex);
}

#include <iostream>  
#include <windows.h>
#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include <boost/asio.hpp>
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"

bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

int main()  
{  
   SetConsoleOutputCP(CP_UTF8); // 输出编码  
   SetConsoleCP(CP_UTF8); // 输入编码  
   try {
	   auto& cfg = ConfigMgr::Inst();
	   auto pool = AsioIOServicePool::GetInstance();
	   boost::asio::io_context io_context;
	   // 异步监控停止
	   boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
	   signals.async_wait([&io_context, pool](auto, auto) {
		   io_context.stop();
		   pool->Stop();
       });
	   auto port_str = cfg["SelfServer"]["port"];
	   CServer s(io_context, atoi(port_str.c_str()));
	   io_context.run(); // 必须有绑定（上一行代码）
   }
   catch (std::exception& e) {
	   std::cerr << "出现错误：" << e.what() << std::endl;
   }
}  

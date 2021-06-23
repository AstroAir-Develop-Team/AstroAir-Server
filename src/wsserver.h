/*
 * wsserver.h
 * 
 * Copyright (C) 2020-2021 Max Qian
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
 
/************************************************* 
 
Copyright: 2020-2021 Max Qian. All rights reserved
 
Author:Max Qian

E-mail:astro_air@126.com
 
Date:2021-2-27
 
Description:Main framework of astroair server
 
**************************************************/

#pragma once

#ifndef _WSSERVER_H_
#define _WSSERVER_H_

#include "config.h"

#define _(str)  gettext(str)

#ifdef HAS_WEBSOCKET
	#include <websocketpp/config/asio.hpp>
	#include <websocketpp/server.hpp>
#endif

#ifdef HAS_JSONCPP
	#include <json/json.h>
#endif

#ifdef HAS_NOVA
	#include "libastro.h"
#endif

#include <string>
#include <set>
#include <dirent.h>
#include <vector>
#include <chrono>
#include <atomic>
#include <fstream>

#ifdef HAS_WEBSOCKET
	typedef websocketpp::server<websocketpp::config::asio> airserver;
	typedef std::set<websocketpp::connection_hdl, std::owner_less<websocketpp::connection_hdl>> con_list;
	using websocketpp::lib::placeholders::_1;
	using websocketpp::lib::placeholders::_2;
	using websocketpp::lib::bind;
	using websocketpp::lib::mutex;
	using websocketpp::lib::lock_guard;
	using websocketpp::lib::condition_variable;
	typedef airserver::message_ptr message_ptr;
#endif

namespace AstroAir
{
	class WSSERVER
	{
		public:
			/*WebSocket服务器主体函数*/
			explicit WSSERVER();
			~WSSERVER();
			virtual void on_open(websocketpp::connection_hdl hdl);
			virtual void on_close(websocketpp::connection_hdl hdl);
			virtual void on_message(websocketpp::connection_hdl hdl,message_ptr msg);
			virtual void send(std::string payload);
			virtual void stop();
			virtual bool is_running();
			/*运行服务器*/
			virtual void run(int port);
		public:
			virtual bool Connect(std::string Device_name);
			virtual bool Disconnect();
			virtual std::string ReturnDeviceName();
		/*电调*/
		public:
			virtual bool MoveTo(int TargetPosition);
		/*滤镜轮*/
		public:
			virtual bool FilterMoveTo(int TargetPosition);
		protected:
			/*转化Json信息*/
			void readJson(std::string message);
			/*搜索目标*/
			void SearchTarget(std::string TargetName);
			/*解析*/
			//void SolveActualPosition(bool IsBlind,bool IsSync);
			/*相机拍摄计数*/
			void ImagineThread();
			/*WebSocket服务器功能性函数*/
			void SetDashBoardMode();
			/*获取配置文件*/
			void GetAstroAirProfiles();
			/*设置配置文件*/
			void SetProfile(std::string File_Name);
			void SetupConnect(int timeout);
			void SetupDisconnect(int timeout);
			void GetFilterConfiguration();
			void GetListAvalaibleSequence();
			void RunSequence(std::string SequenceFile);
			void GetListAvalaibleDragScript();
			/*处理正确返回信息*/
			void SetupConnectSuccess();
			void SetupDisconnectSuccess();
			void EnvironmentDataSend();
			//void StartExposureSuccess();
			//void AbortExposureSuccess();
			//void ShotRunningSend(int ElapsedPerc,int id);
			//void newJPGReadySend();
			//void SearchTargetSuccess(std::string RA,std::string DEC,std::string Name,std::string OtherName,std::string Type,std::string MAG);
			//void SolveActualPositionSuccess();
			/*处理错误信息函数*/
			void SetupConnectError(int id);
			//void StartExposureError();
			//void AbortExposureError();
			//void SearchTargetError(int id);
			//void SolveActualPositionError();
			void RunSequenceError(std::string error);
			/*网页日志*/
			
			void UnknownMsg();
			void UnknownDevice(int id,std::string message);
			void ClientNumError();
			void ErrorCode();
			void Polling();
		private:
			airserver m_server;
			con_list m_connections;
			virtual bool LoadConfigure();
			Json::Value root;
			Json::String errs;
			Json::CharReaderBuilder reader;
			std::string method,json_message,Image_Name;
			std::string Camera,Mount,Focus,Filter,Guide;
			std::string Camera_name,Mount_name,Focus_name,Filter_name,Guide_name;

			mutex mtx,mtx_action;
			condition_variable m_server_cond,m_server_action;
			/*定义服务器设备参数*/
			WSSERVER *FOCUS,*FILTER,*GUIDE;
			std::string FileName,SequenceTarget,SequenceImageName;
			std::string FileBuf[10];
			int DeviceNum = 0;
			int ClientNum = 0;
			std::string DeviceBuf[5];
			/*服务器设备连接状态参数*/
			std::atomic_bool isConnected;

			std::atomic_bool isMountConnected;
			std::atomic_bool isFocusConnected;
			std::atomic_bool isFilterConnected;
			std::atomic_bool isGuideConnected;
			std::atomic_bool InExposure;
			std::atomic_bool InSequenceRun;
			
	};		
	extern WSSERVER ws;
	extern std::string img_data,SequenceTarget;
	void WebLog(std::string message,int type);
	extern std::atomic_bool isCameraConnected;
	/*服务器配置参数*/
	extern int MaxUsedTime,MaxThreadNumber,MaxClientNumber;		//解析最长时间,最多能同时处理的事件数量,最大客户端数量
	extern std::string TargetRA,TargetDEC,MountAngle;
}

#endif

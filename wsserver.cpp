/*
 * wsserver.cpp 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */
 
/************************************************* 
 
Copyright: 2020-2021 Max Qian. All rights reserved
 
Author:Max Qian

E-mail:astro_air@126.com
 
Date:2021-1-4
 
Description:Main framework of astroair server

Using:Websocketpp<https://github.com/zaphoyd/websocketpp>
      
Using:JsonCpp<https://github.com/open-source-parsers/jsoncpp>

**************************************************/

#include "wsserver.h"
#include "logger.h"
#ifdef HAS_ASI
#include "air-asi/asi_ccd.h"
#endif
#ifdef HAS_QHY
#include "air-qhy/qhy_ccd.h"
#endif
#ifdef HAS_INDI
#include "air-indi/indi_ccd.h"
#endif
namespace AstroAir
{
    /*定义ASI相机*/
    #ifdef HAS_ASI
        ASICCD ASICamera;
    #endif
    /*定义QHY相机*/
    #ifdef HAS_QHY
        QHYCCD QHYCamera;
    #endif
    /*
     * name: WSSERVER()
     * describe: Constructor for initializing server parameters
     * 描述：构造函数，用于初始化服务器参数
     */
    WSSERVER::WSSERVER()
    {
        /*初始化WebSocket服务器*/
        /*加载设置*/
        m_server.set_access_channels(websocketpp::log::alevel::all);
        m_server.clear_access_channels(websocketpp::log::alevel::frame_payload);
        /*初始化服务器*/
        m_server.init_asio();
        /*设置打开事件*/
        m_server.set_open_handler(bind(&WSSERVER::on_open, this , ::_1));
        /*设置关闭事件*/
        m_server.set_close_handler(bind(&WSSERVER::on_close, this , ::_1));
        /*设置事件*/
        m_server.set_message_handler(bind(&WSSERVER::on_message, this ,::_1,::_2));
        /*重置参数*/
        isConnected = false;            //客户端连接状态
        isCameraConnected = false;      //相机连接状态
        isMountConnected = false;       //赤道仪连接状态
        isFocusConnected = false;       //电动调焦座连接状态
        isFilterConnected = false;      //滤镜轮连接状态
        isGuideConnected = false;       //导星软件连接状态
    }
    
    /*
     * name: ~WSSERVER()
     * describe: Destructor
     * 描述：析构函数
     * calls: stop()
     */
    WSSERVER::~WSSERVER()
    {
        if(isConnected ==true)
        {
            stop();
        }
    }

    /*
     * name: on_open(websocketpp::connection_hdl hdl)
     * @param hdl:WebSocket句柄
     * describe: Clear data on server connection
     * 描述：服务器连接时插入句柄
     */
    void WSSERVER::on_open(websocketpp::connection_hdl hdl)
    {
        IDLog("Successfully established connection with client\n");
        m_connections.insert(hdl);
        isConnected = true;
    }
    
    /*
     * name: on_close(websocketpp::connection_hdl hdl)
     * @param hdl:WebSocket句柄
     * describe: Clear data on server disconnection
     * 描述：服务器断开连接时清空数据
     */
    void WSSERVER::on_close(websocketpp::connection_hdl hdl)
    {
        IDLog("Disconnect from client\n");
        m_connections.erase(hdl);
        isConnected = false;
    }
    
    /*
     * name: on_message(websocketpp::connection_hdl hdl,message_ptr msg)
     * @param hdl:WebSocket句柄
     * @param msg：服务器信息
     * describe: Processing information from clients
     * 描述：处理来自客户端的信息
     * calls: readJson(std::string message)
     */
    void WSSERVER::on_message(websocketpp::connection_hdl hdl,message_ptr msg)
    {
        std::string message = msg->get_payload();
        /*将接收到的信息打印至终端*/
        IDLog("Get information from client: %s\n",message.c_str());
        if(DebugMode == true)
        {
            /*将接收到的信息写入文件*/
            IDLog_CMDL(message.c_str());
        }
        /*处理信息*/
        readJson(message);
    }
    
    /*以下三个函数均是用于switch支持string*/
	constexpr hash_t hash_compile_time(char const* str, hash_t last_value = basis)  
    {  
        return *str ? hash_compile_time(str+1, (*str ^ last_value) * prime) : last_value;  
    }  

    hash_t hash_(char const* str)  
    {  
        hash_t ret{basis};  
        while(*str){  
            ret ^= *str;  
            ret *= prime;  
            str++;  
        }  
        return ret;  
    }  
    
    constexpr unsigned long long operator "" _hash(char const* p, size_t)
    {
        return hash_compile_time(p);
    }
    
    /*
     * name: readJson(std::string message)
     * @param message:客户端信息
     * describe: Process information and complete
     * 描述：处理信息并完成对应任务
     * note: This is the heart of the whole process!!!
     */
    void WSSERVER::readJson(std::string message)
    {
        /*运用JsonCpp拆分JSON数组*/
        std::unique_ptr<Json::CharReader>const json_read(reader.newCharReader());
        json_read->parse(message.c_str(), message.c_str() + message.length(), &root,&errs);
        /*将string格式转化为const char*/
        method = root["method"].asString();
        const char* road = method.c_str();
        /*判断客户端需要执行的命令*/
        switch(hash_(road))
        {
            /*返回服务器版本号*/
            case "RemoteSetDashboardMode"_hash:
                SetDashBoardMode();
                break;
            /*返回当前目录下的文件*/
            case "RemoteGetAstroAirProfiles"_hash:
                GetAstroAirProfiles();
                break;
            /*连接设备*/
            case "RemoteSetupConnect"_hash:{
                std::thread t1(&WSSERVER::SetupConnect,this,root["params"]["TimeoutConnect"].asInt());
                t1.detach();
                break;
            }
            /*相机开始拍摄*/
            case "RemoteCameraShot"_hash:{
                std::thread CamThread(&WSSERVER::StartExposure,this,root["params"]["Expo"].asInt(),root["params"]["Bin"].asInt(),root["params"]["IsROI"].asBool(),root["params"]["ROITYPE"].asInt(),root["params"]["ROIX"].asInt(),root["params"]["ROIY"].asInt(),root["params"]["IsSaveFile"].asBool(),root["params"]["FitFileName"].asString(),root["params"]["Gain"].asInt(),root["params"]["Offset"].asInt());
                CamThread.detach();
                break;
            }
            /*轮询，保持连接*/
            case "Polling"_hash:
                Polling();
                break;
            /*默认返回未知信息*/
            default:
                UnknownMsg();
        }
    }
    
    /*
     * name: send(std::string payload)
     * @param message:需要发送的信息
     * describe: Send information to client
     * 描述：向客户端发送信息
     * note: The message must be sent in the format of JSON
     */
    void WSSERVER::send(std::string message)
    {
        for (auto it : m_connections)
        {
            try
            {
                m_server.send(it, message, websocketpp::frame::opcode::text);
            }
            catch (websocketpp::exception const &e)
            {
                IDLog("Unable to send message, please check connection\n");
                IDLog_DEBUG("%s\n",e.what());
            }
            catch (...)
            {
                std::cerr << "other exception" << std::endl;
            }
        }
    }
    
    /*
     * name: send_binary(void const * payload, size_t len)
     * @param image:需要发送的二进制信息
     * @param len:需发送文件的大小
     * describe: Send binary information to client
     * 描述：向客户端发送二进制信息
     * note: This function can send the image to the client and display it again
     */
    void WSSERVER::send_binary(void const * image, size_t len)
    {
        for (auto it : m_connections)
        {
            try
            {
                m_server.send(it, image, len, websocketpp::frame::opcode::binary);
            }
            catch (websocketpp::exception const &e)
            {
                IDLog("Unable to send binary message, please check connection\n");
                std::cerr << e.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "other exception" << std::endl;
            }
        }
    }
    
    /*
     * name: stop()
     * describe: Stop the websocket server
     * 描述：停止WebSocket服务器
     */
    void WSSERVER::stop()
    {
        for (auto it : m_connections)
            m_server.close(it, websocketpp::close::status::normal, "Switched off by user.");
        IDLog("Stop the server..\n");
        m_connections.clear();
        m_server.stop();
        IDLog("Good bye\n");
    }

    /*
     * name: is_running()
     * @return Boolean function:服务器运行状态
     *  -false: server is not running||服务器不在运行
     *  -true: server is running||服务器正在运行
     */
    bool WSSERVER:: is_running()
    {
        return m_server.is_listening();
    }
    
    /*
     * name: run(int port)
     * @param port:服务器端口
     * describe: This is used to start the websocket server
     * 描述：启动WebSocket服务器
     */
    void WSSERVER::run(int port)
    {
        try
        {
            IDLog("Start the server..\n");
            m_server.listen(port);
            m_server.start_accept();
            m_server.run();
        }
        catch (websocketpp::exception const &e)
        {   
            IDLog("Unable to start the server\nThe reason is:");
            IDLog_DEBUG("%s\n",e.what());
            //std::cerr << e.what() << std::endl;
            
        }
        catch (...)
        {
            std::cerr << "other exception" << std::endl;
        }
    }
    
    /*
     * name: SetDashBoardMode()
     * describe: This is used to initialize the connection and send the version number.
     * 描述：初始化连接，并发送版本号
     * calls: send(std::string message)
     * calls: IDLog(const char *fmt, ...)
     */
    void WSSERVER::SetDashBoardMode()
	{
        Json::Value Root;
		Root["result"] = Json::Value(1);
		Root["code"] = Json::Value();
		Root["Event"] = Json::Value("Version");
		Root["AIRVersion"] = Json::Value("2.0.0");
		json_messenge = Root.toStyledString();
		send(json_messenge);
	}

    /*
     * name: GetAstroAirProfiles()
     * describe: Get the local file name and upload it to the client
     * 描述：获取本地文件名称并上传至客户端
     * describe: Gets the specified suffix file name in the folder
	 * 描述：获取文件夹中指定后缀文件名称
     * calls: send()
	 * note:The suffix of get file should be .air or .json
     */
    void WSSERVER::GetAstroAirProfiles()
    {
        /*寻找当前文件夹下的所有文件*/
        struct dirent *ptr;      
		DIR *dir;  
		std::string PATH = "./";  
		dir=opendir(PATH.c_str());   
		std::vector<std::string> files;  
		while((ptr=readdir(dir))!=NULL)  
		{  
			if(ptr->d_name[0] == '.')  
				continue;  
			files.push_back(ptr->d_name);  
		}  
		for (int i = 0; i < files.size(); ++i)  
		{  
			std::cout << files[i] << std::endl;  
		}  
		closedir(dir);
        /*整合信息并发送至客户端*/
        Json::Value Root,list,name;
        Root["result"] = Json::Value(1);
		Root["code"] = Json::Value();
		Root["Event"] = Json::Value("RemoteActionResult");
        Root["UID"] = Json::Value("RemoteGetAstroAirProfiles");
        name["name"] = Json::Value("config.air");
        Root["list"] = Json::Value("config2.air");
        list["list"] = name;
        Root["ParamRet"] = list;
        Root["ActionResultInt"] = Json::Value(4);
        json_messenge = Root.toStyledString();
        send(json_messenge);
    }
    
    /*
     * name: SetupConnect(int timeout)
     * @param timeout:连接相机最长时间
     * describe: All connection profiles in the device
     * 描述：连接配置文件中的所有设备
     * calls: Connect(std::string Device_name)
     * calls: IDLog(const char *fmt, ...)
     * calls: IDLog_DEBUG(const char *fmt, ...)
     * calls: UnknownCamera()
     * calls: UnknownMount()
     * note: If it times out, an error message is returned
     */
    void WSSERVER::SetupConnect(int timeout)
    {
        /*读取config.air配置文件，并且存入参数中*/
        std::string line,jsonStr;
        std::ifstream in("config.air", std::ios::binary);
        /*打开文件*/
        if (!in.is_open())
        {
            IDLog("Unable to open configuration file\n");
            IDLog_DEBUG("Unable to open configuration file\n");
            return;
        }
        /*将文件转化为string格式*/
        while (getline(in, line))
        {
            jsonStr.append(line);
        }
        /*关闭文件*/
        in.close();
        /*将读取出的json数组转化为string*/
        std::unique_ptr<Json::CharReader>const json_read(reader.newCharReader());
        json_read->parse(jsonStr.c_str(), jsonStr.c_str() + jsonStr.length(), &root,&errs);
        bool connect_ok = false;
        /*连接指定品牌的指定型号相机*/
        bool camera_ok = false;
        bool Has_Camera = false;
        Camera = root["camera"]["brand"].asString();
        Camera_name = root["camera"]["name"].asString();
        if(!Camera.empty() && !Camera_name.empty())
        {
            Has_Camera = true;
            for(int i=1;i<=3;i++)
            {
                const char* a = Camera.c_str();
                switch(hash_(a))
                {
                    #ifdef HAS_ASI
                    case "ZWOASI"_hash:{
                        CCD = &ASICamera;
                        camera_ok = CCD->Connect(Camera_name);
                        break;
                    }
                    #endif
                    #ifdef HAS_QHY
                    case "QHYCCD"_hash:{
                        CCD = &QHYCamera;
                        camera_ok = CCD->Connect(Camera_name);
                        break;
                    }
                    #endif
                    #ifdef HAS_INDI
                    case "INDI"_hash:
                        CCD = &INDICamera;
                        camera_ok = CCD->Connect(Camera_name);
                        break;
                    #endif
                    default:
                        UnknownDevice(301,"Unknown camera");
                }
                if(camera_ok == true)
                {
                    isCameraConnected = true;
                    break;
                }
                if(i == 3&& camera_ok == false)
                {
                    SetupConnectError("Unable to connect camera");
                    break;
                }
                sleep(10);
            }
        }
        /*判断相机是否连接成功*/
        if(Has_Camera == true)
        {
            if(camera_ok == true)
                connect_ok == true;
            else
                connect_ok == false;
        }
        /*连接指定品牌的指定型号赤道仪*/
        bool mount_ok = false;
        bool Has_Mount = false;
        Mount = root["mount"]["brand"].asString();
        Mount_name = root["mount"]["name"].asString();
        if(!Mount.empty() && !Mount_name.empty())
        {
            Has_Mount = true;
            for(int i=1;i<=3;i++)
            {
                const char* a = Mount.c_str();
                switch(hash_(a))
                {
                    #ifdef HAS_IOPTRON
                    case "iOptron"_hash:{
                        MOUNT = &iOptronMount;
                        monut_ok = MOUNT->Connect(Mount_name);
                        break;
                    }
                    #endif
                    #ifdef HAS_SKYWATCHER
                    case "SkyWatcher"_hash:{
                        MOUNT = &SkyWatcherMount;
                        mount_ok = MOUNT->Connect(Mount_name);
                        break;
                    }
                    #endif
                    #ifdef HAS_INDI
                    case "INDIMount"_hash:{
                        MOUNT = &INDIMount
                        mount_ok = MOUNT->Connect(Mount_name);
                        break;
                    }
                    #endif
                    default:
                        UnknownDevice(302,"Unknown mount");
                }
                if(mount_ok == true)
                {
                    isMountConnected = true;
                    break;
                }
                if(i == 3&& mount_ok == false)
                {
                    SetupConnectError("Unable to connect mount");
                    break;
                }
                sleep(10);
            }
        }
        /*判断赤道仪是否连接成功*/
        if(Has_Mount == true)
        {
            if(mount_ok == true)
                connect_ok == true;
            else
                connect_ok == false;
        }
        /*连接指定品牌的指定型号电动调焦座*/
        bool focus_ok = false;
        bool Has_Focus = false;
        Focus = root["focus"]["brand"].asString();
        Focus_name = root["focus"]["name"].asString();
        if(!Focus.empty() && !Focus_name.empty())
        {
            Has_Focus = true;
            for(int i = 0;i<=3;i++)
            {
                const char* a = Focus.c_str();
                switch(hash_(a))
                {
                    #ifdef HAS_ASIEAF
                    case "ASIEAF"_hash:{
                        FOCUS = &EAFFocus;
                        focus_ok = FOCUS->Connect(Focus_name);
                        break;
                    }
                    #endif
                    #ifdef HAS_GRUS
                    case "Grus"_hash:{
                        FOCUS = &GRUSFocus;
                        focus_ok = FOCUS->Connect(Focus_name);
                        break;
                    }
                    #endif
                    #ifdef HAS_INDI
                    case "INDIFocus"_hash:{
                        FOCUS = &INDIFocus;
                        focus_ok = FOCUS->Connect(Focus_name);
                        break;
                    }
                    #endif
                    default:
                        UnknownDevice(303,"Unknown focus");
                }
                if(focus_ok == true)
                {
                    isFocusConnected = true;
                    break;
                }
                if(i == 3&& focus_ok == false)
                {
                    SetupConnectError("Unable to connect focus");
                    break;
                }
                sleep(10);
            }
        }
        /*判断电动调焦座是否连接成功*/
        if(Has_Focus == true)
        {
            if(focus_ok == true)
                connect_ok == true;
            else
                connect_ok == false;
        }
        /*连接指定品牌的指定型号滤镜轮*/
        bool filter_ok = false;
        bool Has_Filter = false;
        Filter = root["filter"]["brand"].asString();
        Filter_name = root["filter"]["name"].asString();
        if(!Filter.empty() && !Filter_name.empty())
        {
            Has_Filter = true;
            for(int i = 1;i<=3;i++)
            {
                const char* a = Filter.c_str();
                switch(hash_(a))
                {
                    #ifdef HAS_ASIEFW
                    case "ASIEFW"_hash:{
                        FILTER = &ASIFilter;
                        filter_ok = FILTER->Connect(Filter_name);
                        break;
                    }
                    #endif
                    #ifdef HAS_QHYCFW
                    case "QHYCFW"_hash:{
                        FILTER = &QHYFilter
                        filter_ok = FILTER->Connect(Filter_name);
                        break;
                    }
                    #endif
                    #ifdef HAS_INDI
                    case "INDIFilter"_hash:{
                        FILTER = &INDIFilter;
                        filter_ok = FILTER->Connect(Filter_name);
                        break; 
                    }
                    #endif
                    default:
                        UnknownDevice(304,"Unknown filter");
                }
                if(filter_ok == true)
                {
                    isFilterConnected = true;
                    break;
                }
                if(i == 3&& filter_ok == false)
                {
                    SetupConnectError("Unable to connect filter");
                    break;
                }
                sleep(10);
            }
        }
        /*判断滤镜轮是否连接成功*/
        if(Has_Filter == true)
        {
            if(filter_ok == true)
                connect_ok == true;
            else
                connect_ok == false;
        }
        /*连接指定品牌的指定型号导星软件*/
        bool guide_ok = false;
        bool Has_Guide = false;
        Guide = root["Guide"]["brand"].asString();
        Guide_name = root["Guide"]["name"].asString();
        if(!Guide.empty() && !Guide_name.empty())
        {
            Has_Guide = true;
            for(int i = 1;i<=3;i++)
            {
                const char* a = Guide.c_str();
                switch(hash_(a))
                {
                    #ifdef HAS_PHD2
                    case "PHD2"_hash:{
                        GUIDE = &PHD2;
                        guide_ok = GUIDE->Connect(Guide_name);
                        break;
                    }
                    #endif
                    #ifdef HAS_LINGUIDER
                    case "LinGuider"_hash:{
                        GUIDE = &LinGuider
                        guide_ok = GUIDE->Connect(Guide_name);
                        break;
                    }
                    #endif
                    default:
                        UnknownDevice(305,"Unknown guide server");
                }
                if(guide_ok == true)
                {
                    isGuideConnected = true;
                    break;
                }
                if(i == 3&& guide_ok == false)
                {
                    SetupConnectError("Unable to connect giude software");
                    break;
                }
                sleep(10);
            }
        }
        /*判断导星软件是否连接成功*/
        if(Has_Guide == true)
        {
            if(guide_ok == true)
                connect_ok == true;
            else
                connect_ok == false;
        }
        /*判断设备是否完全连接成功*/
        if(connect_ok == true)
            SetupConnectSuccess();
        return;
    }
    
    /*
     * name: Connect(std::string Device_name)
     * @param Device_name:连接相机名称
     * describe: Connect the camera
     * 描述： 连接相机
     * calls: IDLog(const char *fmt, ...)
     * calls: IDLog_DEBUG(const char *fmt, ...)
	 * note:This function should not be executed normally
     */
    bool WSSERVER::Connect(std::string Device_name)
    {
        IDLog("Try to establish a connection with %s,Should never get here.\n",Device_name);
        IDLog_DEBUG("Try to establish a connection with %s,Should never get here.\n",Device_name);
        return true;
    }
    
    /*
     * name: Disconnect()
     * describe: Disconnect from camera
     * 描述：与相机断开连接
     * calls: IDLog(const char *fmt, ...)
     * calls: IDLog_DEBUG(const char *fmt, ...)
     * note: This function should not be executed normally
     */
    bool WSSERVER::Disconnect()
    {
        IDLog("Try to disconnect from %s,Should never get here.\n",Camera_name);
        IDLog_DEBUG("Try to disconnect from %s,Should never get here.\n",Camera_name);
        return true;
    }
    
    /*
     * name: StartExposure(float exp,int bin,bool is_roi,int roi_type,int roi_x,int roi_y,bool is_save,std::string fitsname,int gain,int offset)
     * @param exp:相机曝光时间
     * @param bin:像素合并
     * @param is_roi:是否开启ROI模式
     * @param roi_x:ROIx轴起点
     * @param roi_y:ROIy轴起点
     * @param is_save:是否保存图像
     * @param fitsname:保存图像名称
     * @param gain:相机增益
     * @param offset:相机偏置
     * describe: Start exposure
     * 描述：开始曝光
	 * calls: StartExposure(float exp,int bin,bool is_roi,int roi_type,int roi_x,int roi_y,bool is_save,std::string fitsname,int gain,int offset)
     * calls: IDLog(const char *fmt, ...)
     * calls: IDLog_DEBUG(const char *fmt, ...)
	 * note:This function should not be executed normally
     */
    bool WSSERVER::StartExposure(float exp,int bin,bool is_roi,int roi_type,int roi_x,int roi_y,bool is_save,std::string fitsname,int gain,int offset)
    {
		bool camera_ok = false;
		if (camera_ok = CCD->StartExposure(exp, bin, is_roi, roi_type, roi_x, roi_y, is_save, fitsname, gain, offset) != true)
		{
			StartExposureError();
			return false;
		}
		StartEcpo
        return true;
    }
    
    /*
     * name: AbortExposure()
     * describe: Abort exposure
     * 描述：停止曝光
     * calls: IDLog(const char *fmt, ...)
     * calls: IDLog_DEBUG(const char *fmt, ...)
     * note: This function should not be executed normally
     */
    bool WSSERVER::AbortExposure()
    {
        IDLog("Try to stop exposure,Should never get here.\n");
        IDLog_DEBUG("Try to stop exposure,Should never get here.\n");
        return true;
    }
    
    void WSSERVER::SetupConnectSuccess()
    {
        
    }
    
    /*
     * name: SetupConnectError(std::string message)
     * @prama message:需要返回至客户端的错误信息
     * describe: Error handling connection to device
     * 描述：处理连接设备时的错误
     * calls: IDLog(const char *fmt, ...)
     * calls: IDLog_DEBUG(const char *fmt, ...)
     * calls: send()
     */
    void WSSERVER::SetupConnectError(std::string message)
    {
        IDLog("Unable to connect device\n");
        IDLog_DEBUG("Unable to connect device\n");
        /*整合信息并发送至客户端*/
        Json::Value Root,error;
        Root["result"] = Json::Value(1);
		Root["code"] = Json::Value();
        Root["id"] = Json::Value(201);
        error["message"] = Json::Value(message);
        Root["error"] = error;
        json_messenge = Root.toStyledString();
        send(json_messenge);
    }
    
	void WSSERVER::StartExposureSuccess()
	{

	}

    void WSSERVER::StartExposureError()
    {
        
    }
    
    /*
     * name: UnknownMsg()
     * describe: Processing unknown information from clients
     * 描述：处理来自客户端的未知信息
     * calls: IDLog(const char *fmt, ...)
     * calls: IDLog_DEBUG(const char *fmt, ...)
     * calls: send()
	 * note:If this function is executed, an error will appear on the web page
     */
    void WSSERVER::UnknownMsg()
    {
        IDLog("An unknown message was received from the client\n");
        IDLog_DEBUG("An unknown message was received from the client\n");
        /*整合信息并发送至客户端*/
        Json::Value Root,error;
        Root["result"] = Json::Value(1);
		Root["code"] = Json::Value();
        Root["id"] = Json::Value(403);
        error["message"] = Json::Value("Unknown information");
        Root["error"] = error;
        json_messenge = Root.toStyledString();
        send(json_messenge);
    }
    
    /*
     * name: UnknownDevice(int id,std::string message);
     * describe: Process the unknown device information and return to the client
     * 描述：处理未知设备信息，并返回至客户端
     * calls: IDLog(const char *fmt, ...)
     * calls: send()
	 * note:Execute this function if an unknown device is found
     */
    void WSSERVER::UnknownDevice(int id,std::string message)
    {
        IDLog("An unknown device was found,please check the connection\n");
        /*整合信息并发送至客户端*/
        Json::Value Root,error;
        Root["result"] = Json::Value(1);
		Root["code"] = Json::Value();
        Root["id"] = Json::Value(id);
        error["message"] = Json::Value(message);
        Root["error"] = error;
        json_messenge = Root.toStyledString();
        send(json_messenge);
    }
    
    /*
     * name: Polling()
     * describe: The client remains connected to the server
     * 描述：客户端与服务器保持连接
     * calls: send()
	 * note:This function is most commonly used
     */
    void WSSERVER::Polling()
    {
        Json::Value Root;
        Root["result"] = Json::Value(1);
		Root["code"] = Json::Value();
        Root["Event"] = Json::Value("Polling");
        json_messenge = Root.toStyledString();
        send(json_messenge);
    }
        
}

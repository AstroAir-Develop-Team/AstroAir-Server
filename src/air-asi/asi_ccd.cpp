/*
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

Date:2020-12-11

Description:ZWO camera driver

**************************************************/

#include "asi_ccd.h"
#include "../logger.h"

namespace AstroAir
{
    /*
     * name: ASICCD()
     * describe: Initialization, for camera constructor
     * 描述：构造函数，用于初始化相机参数
     */
    ASICCD::ASICCD()
    {
		CamNumber = 0;
		CamId = 0;
		isConnected = false;
		InVideo = false;
		InExposure = false;
		InCooling = false;
    }
    
    /*
     * name: ~ASICCD()
     * describe: Destructor
     * 描述：析构函数
     * calls: Disconnect()
     * note: Ensure camera safety
     */
    ASICCD::~ASICCD()
    {
		if (isConnected == true)
		{
			Disconnect();
		}
    }
    
    /*
     * name: Connect(std::string Device_name)
     * @param Device_name:连接相机名称
     * describe: Connect the camera
     * 描述： 连接相机
     * calls: ASIGetNumOfConnectedCameras()
     * calls: ASIGetCameraProperty()
     * calls: ASIOpenCamera()
     * calls: ASIInitCamera()
     * calls: UpdateCameraConfig()
     * calls: IDLog()
     * @return true: Camera connected successfully
     * @return false: Failed to connect camera
     * note: Do not connect the camera repeatedly
     */
    bool ASICCD::Connect(std::string Device_name)
    {
		/*获取已连接相机数量*/
		CamNumber = ASIGetNumOfConnectedCameras();
		if(CamNumber <= 0)
		{
			IDLog("ASI camera not found, please check the power supply or make sure the camera is connected.\n");
			return false;
		}
		else
		{
			for(int i = 0;i < CamNumber;i++)
			{
				/*获取相机信息*/
				if(ASIGetCameraProperty(&ASICameraInfo, i) != ASI_SUCCESS)
				{
					IDLog("Unable to get %s configuration information, please check program permissions.\n",ASICameraInfo.Name);
					return false;
				}
				else
				{
					if(ASICameraInfo.Name == Device_name)
					{
						IDLog("Find %s.\n",ASICameraInfo.Name);
						CamId = ASICameraInfo.CameraID;
						CamName[CamId] = ASICameraInfo.Name;
						/*打开相机*/
						if(ASIOpenCamera(CamId) != ASI_SUCCESS)		
						{
							IDLog("Unable to turn on the %s.\n",CamName[CamId]);
							return false;
						}
						else
						{
							/*初始化相机*/
							if(ASIInitCamera(CamId) != ASI_SUCCESS)	
							{
							IDLog("Unable to initialize connection to camera.\n");
							return false;
							}
							else 
							{
							isConnected = true;
							IDLog("Camera turned on successfully\n");
							/*获取连接相机配置信息，并存入参数*/
							UpdateCameraConfig();
							return true;
							}
						}
					}
					else
					{
					IDLog("This is not a designated camera, try to find the next one.\n");
					}
				}
			}
			IDLog("The specified camera was not found. Please check the camera connection");
			return false;
		}
		return false;
    }
    
    /*
     * name: Disconnect()
     * describe: Disconnect from camera
     * 描述：与相机断开连接
     * calls: ASIStopVideoCapture()
     * calls: ASIStopExposure()
     * calls: SaveConfig()
     * calls: ASICloseCamera()
     * calls: IDLog()
     * note: Please stop all work before turning off the camera
     */
    bool ASICCD::Disconnect()
    {
		/*在关闭相机之前停止所有任务*/
		if(InVideo == true)
		{
			if(ASIStopVideoCapture(CamId) != ASI_SUCCESS);		//停止视频拍摄
			{
			IDLog("Unable to stop video capture, please try aGain.\n");
			return false;
			}
			IDLog("Stop video capture.\n");
		}
		if(InExposure == true)
		{
			if(ASIStopExposure(CamId) != ASI_SUCCESS)		//停止曝光
			{
			IDLog("Unable to stop exposure, please try aGain.\n");
			return false;
			}
			IDLog("Stop exposure.\n");
		}
		/*在关闭相机之前保存设置*/
		//SaveConfig();
		/*关闭相机*/
		if(ASICloseCamera(CamId) != ASI_SUCCESS)		//关闭相机
		{
			IDLog("Unable to turn off the camera, please try aGain");
			return false;
		}
		IDLog("Disconnect from camera\n");
		return true;
    }
    
    /*
     * name: UpdateCameraConfig()
     * describe: Get the required parameters of the camera
     * 描述：获取相机所需参数
     * @return ture: meaningless
     * note: These parameters are very important and related to the following program
     */
    bool ASICCD::UpdateCameraConfig()
    {
		/*判断是否为彩色相机*/
		if(ASICameraInfo.IsColorCam == true)
			isColorCamera = true;
		/*判断是否为制冷相机*/
		if(ASICameraInfo.IsCoolerCam == true)
			isCoolCamera = true;
		/*判断是否为导星相机*/
		if(ASICameraInfo.ST4Port == true)
			isGuideCamera = true;
		/*获取相机最大画幅*/
		iMaxWidth = ASICameraInfo.MaxWidth;
		iMaxHeight = ASICameraInfo.MaxHeight;
		CamWidth = iMaxWidth;
		CamHeight = iMaxHeight;
		IDLog("Camera information obtained successfully.\n");
		return true;
    }
    
    /*
     * name: SetTemperature(double temperature)
     * describe: Set camera cooling temperature
     * 描述：设置相机制冷温度
     * @param temperature: 目标温度
     * @return ture: 成功设置温度
     * @return false：无法设置温度
     * calls: ActiveCool()
     * calls: ASISetControlValue()
     * calls: IDLog()
     * note: The temperature of the camera is unlimited. Please pay attention to the limit
     */
    bool ASICCD::SetTemperature(double temperature)
    {
		/*判断输入温度是否合理*/
		if(temperature < -50 ||temperature > 40)
		{
			IDLog("The temperature setting is unreasonable, please reset it.\n");
			return false;
		}
		/*检查是否可以制冷*/
		if(ActiveCool(true) == false)
		{
			IDLog("Unable to start camera cooling, please check the power connection.\n");
			return false;
		}
		/*转化温度参数*/
		long TargetTemp;
		if (temperature > 0.5)
			TargetTemp = static_cast<long>(temperature + 0.49);
		else if (temperature  < 0.5)
			TargetTemp = static_cast<long>(temperature - 0.49);
		else
			TargetTemp = 0;
		/*设置相机温度*/
		if(ASISetControlValue(CamId,ASI_TEMPERATURE,TargetTemp,ASI_FALSE) != ASI_SUCCESS)
		{
			IDLog("Unable to set camera temperature.\n");
			return false;
		}
		TemperatureRequest = temperature;
		IDLog("Camera cooling temperature set successfully.\n");
		return true;
    }
    
    /*
     * name: ActiveCool(bool enable)
     * describe: Start camera cooling
     * 描述：启动相机制冷
     * @param enable: 制冷状态
     * @return ture: 成功开启制冷
     * @return false：无法开启制冷
     * calls: ASISetControlValue()
     * calls: IDLog()
     */
    bool ASICCD::ActiveCool(bool enable)
    {
		if(isCoolCamera == true)
		{
			if(ASISetControlValue(CamId,ASI_COOLER_ON,enable ? ASI_TRUE : ASI_FALSE,ASI_FALSE) != ASI_SUCCESS)
			{
			IDLog("Unable to turn on refrigeration, please check the power supply.\n");
			return false;
			}
			InCooling = true;
			IDLog("Cooling is in progress. Please wait.\n");
		}
		return true;
    }
    
    /*
     * name: StartExposure(int exp,int bin,bool IsSave,std::string FitsName,int Gain,int Offset)
     * describe: Start camera exposure
     * 描述：相机开始曝光
     * @param exp: 曝光时间
     * @param bin:像素合并模式
     * @param IsSave:是否保存图像
     * @param FitsName:图像名称
     * @param Gain:相机增益
     * @param Offset:相机偏置
     * calls: ASISetControlValue()
     * calls: IDLog()
     * calls: ASIStartExposure()
     * calls: ASIGetExpStatus()
     */
    bool ASICCD::StartExposure(int exp,int bin,bool IsSave,std::string FitsName,int Gain,int Offset)
    {
		std::lock_guard<std::mutex> lock(condMutex);
		const long blink_duration = exp * 1000000;
		IDLog("Blinking %ld time(s) before exposure\n", blink_duration);
		errCode = ASISetControlValue(CamId, ASI_EXPOSURE, blink_duration, ASI_FALSE);
		if(errCode != ASI_SUCCESS)
		{
			IDLog("Failed to set blink exposure to %ldus, error %d\n", blink_duration, errCode);
			return false;
		}
		else
		{
			if(SetCameraConfig(bin,Gain,Offset) != true)
			{
				IDLog("Failed to set camera configure\n");
				return false;
			}
			else
			{
				errCode = ASIStartExposure(CamId, ASI_TRUE);
				if(errCode != ASI_SUCCESS)
				{
					IDLog("Failed to start blink exposure, error %d,try it aGain\n", errCode);
				}
				else
				{
					InExposure = true;
					do
					{
						usleep(100000);
						errCode = ASIGetExpStatus(CamId, &expStatus);
					}
					while (errCode == ASI_SUCCESS && expStatus == ASI_EXP_WORKING);
					if (errCode != ASI_SUCCESS || expStatus != ASI_EXP_SUCCESS)
					{
						IDLog("Blink exposure failed, error %d, status %d", errCode, expStatus);
						return false;
					}
					InExposure = false;
                }
            }
        }
        if(IsSave == true)
        {
			IDLog("Finished exposure and save image locally\n");
			if(SaveImage(FitsName) != true)
			{
				IDLog("Could not save image correctly,please check the config\n");
				return false;
			}
			else
			{
				IDLog("Save image %s successfully\n",FitsName);
			}
		}
		return true;
    }
    
    /*
     * name: AbortExposure()
     * describe: Stop camera exposure
     * 描述：停止相机曝光
     * @return ture: 成功停止曝光
     * @return false：无法停止曝光
     * calls: setThreadRequest()
     * calls: waitUntil()
     * calls: ASIStopExposure()
     * calls: IDLog()
     */
    bool ASICCD::AbortExposure()
    {
		IDLog("Aborting camera exposure...");
		if(ASIStopExposure(CamId) != ASI_SUCCESS)
		{
			IDLog("Unable to stop camera exposure,please try again.\n");
			return false;
		}
		InExposure = false;
		return true;
    }
    
    /*
     * name: SetCameraConfig(long Bin,long Gain,long Offset)
     * describe: set camera cinfig
     * 描述：设置相机参数
     * calls: IDLog()
     * calls: ASISetControlValue()
     * calls: ASISetROIFormat()
     */
    bool ASICCD::SetCameraConfig(long Bin,long Gain,long Offset)
    {
		errCode = ASI_SUCCESS;
		errCode = ASISetControlValue(CamId, ASI_GAIN, Gain, ASI_FALSE);
		if(errCode != ASI_SUCCESS)
		{
			IDLog("Unable to set camera gain,error code is %d\n",errCode);
			return false;
		}
		errCode = ASISetControlValue(CamId, ASI_BRIGHTNESS, Offset, ASI_FALSE);
		if(errCode != ASI_SUCCESS)
		{
			IDLog("Unable to set camera offset,error code is %d\n",errCode);
			return false;
		}
		CamWidth = iMaxWidth/Bin;
		CamHeight = iMaxHeight/Bin;
		errCode = ASISetROIFormat(CamId, CamWidth , CamHeight , Bin, (ASI_IMG_TYPE)Image_type);
		if(errCode != ASI_SUCCESS)
		{
			IDLog("Unable to set camera offset,error code is %d\n",errCode);
			return false;
		}
		return true;
    }
    
    /*
     * name: SaveImage(std::string FitsName)
     * describe: Save Fits images
     * 描述：存储Fits图像
     * calls: ASIGetDataAfterExp()
     * calls: fits_create_file()
     * calls: fits_create_img()
     * calls: fits_update_key()
     * calls: fits_write_img()
     * calls: fits_close_file()
     * calls: fits_report_error()
     */
    bool ASICCD::SaveImage(std::string FitsName)
    {
		if(InExposure == false && InVideo == false)
		{	
			#ifdef HAS_FITSIO
				int FitsStatus;		//cFitsio状态
				long imgSize = CamWidth*CamHeight*(1 + (Image_type==ASI_IMG_RAW16));		//设置图像大小
				imgBuf = new unsigned char[imgSize];		//图像缓冲区大小
				ASIGetDataAfterExp(CamId, imgBuf, imgSize);		//曝光后获取图像信息
				naxis = 2;
				long naxes[2] = {CamWidth,CamHeight};			
				fits_create_file(&fptr, FitsName, &FitsStatus);		//创建Fits文件
				if(Image_type==ASI_IMG_RAW16)		//创建Fits图像
					fits_create_img(fptr, USHORT_IMG, naxis, naxes, &FitsStatus);		//16位
				else
					fits_create_img(fptr, BYTE_IMG,   naxis, naxes, &FitsStatus);		//8位或12位
				strcpy(datatype, "TSTRING");
				strcpy(keywords, "Camera");
				strcpy(value,CamName[CamId]);
				strcpy(description, "ZWOASI");
				if(strcmp(datatype, "TSTRING") == 0)		//写入Fits图像头文件
				{
					fits_update_key(fptr, TSTRING, keywords, value, description, &FitsStatus);
				}
				if(Image_type==ASI_IMG_RAW16)		//将缓存图像写入SD卡
					fits_write_img(fptr, TUSHORT, fpixel, imgSize, &imgBuf[0], &FitsStatus);		//16位
				else
					fits_write_img(fptr, TBYTE, fpixel, imgSize, &imgBuf[0], &FitsStatus);		//8位或12位
				fits_close_file(fptr, &FitsStatus);		//关闭Fits图像
				fits_report_error(stderr, FitsStatus);		//如果有错则返回错误信息
			#endif
		}
		if(imgBuf)
			delete[] imgBuf;		//删除图像缓存
		return true;
	}
}





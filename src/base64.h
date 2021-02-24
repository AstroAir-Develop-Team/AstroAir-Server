/*
 * base64.h
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
 
Date:2021-2-24
 
Description:Base64 Library
 
**************************************************/

#pragma once

#ifndef _BASE64_H_
#define _BASE64_H_

#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

namespace AstroAir
{
    /*Base64编码*/
    std::string base64Encode(const unsigned char* Data, int DataByte);
    /*Mat格式转化为Base64格式*/
	std::string Mat2Base64(const cv::Mat &img, std::string imgType);
    /*Base64解码*/
    std::string base64Decode(const char* Data, int DataByte);
    /*Base64格式转化为Mat*/
    cv::Mat Base2Mat(std::string &base64_data);
    /*RSA加密*/
    std::string rsa_pub_encrypt(const std::string &clearText, const std::string &pubKey);
    /*RSA解密*/
    std::string rsa_pri_decrypt(const std::string &cipherText, const std::string &priKey);
}

#endif

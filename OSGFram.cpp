#pragma once
#include <osgEarthAnnotation/PlaceNode>
#include "OSGFram.h"
#include <string>
#include "Utils.h"
#include <osgEarthAnnotation/LabelNode>
#include <osgEarth/ECEF>
#include <osgEarthUtil/AutoClipPlaneHandler>
#include <osgUtil/Tessellator>
#include <osg/LineWidth>
#include "tinyxml.h"
#include "tinystr.h"
#include <vector>
#include "DatabaseSql.h"
#include <string>
#include "MaterialInfo.h"
using namespace std;
using namespace osgEarth::Util;
using namespace osgEarth::Annotation;
using namespace System::Runtime::InteropServices;
using namespace System::Text::RegularExpressions;
using namespace System;
using namespace ProjectOil;
enum osgFrame::DataType
{
	漏油线终点 = 0,
	隧道,
	穿河点,
	高后果区,
	物资站,
	拦油点,
	水电站
};
osgFrame::osgFrame(HWND hWnd)
{
	m_hWnd = hWnd;
}
osgFrame::~osgFrame()
{
}
void osgFrame::InitOSG()
{
	InitSceneGraph();
	InitCamera();
	InitOSGEarth();
}
void osgFrame::InitSceneGraph()
{
	mRoot = new osg::Group();
	displaysettings = new osg::DisplaySettings();
	//解决osg路径与文件名中的中文字符问题
	setlocale(LC_ALL, "chs");
	osg::ref_ptr<osg::Node> mp = osgDB::readNodeFile("../MyEarth2.earth");
	setlocale(LC_ALL, "C");
	mRoot->addChild(mp);
	mapNode = dynamic_cast<osgEarth::MapNode*>(mp.get());
	osg::ref_ptr<osg::Node> node_guojie = osgDB::readNodeFile("../ive/jiexian/guojie3.ive");
	OilUtils::AddIveNode(node_guojie, mRoot);
	osg::ref_ptr<osg::Node> node_shengjie = osgDB::readNodeFile("../ive/jiexian/shengjie2.ive");
	OilUtils::AddIveNode(node_shengjie, mRoot);
	osg::ref_ptr<osg::Node> node_shengjie1 = osgDB::readNodeFile("../ive/jiexian/shengjie1.ive");
	OilUtils::AddIveNode(node_shengjie1, mRoot);
	//可拾取节点
	OilNode = new osg::Group();
	/***加载公共数据***/
	//贵阳方向箭头
	arrow = osgDB::readNodeFile("../ive/箭头/箭头.ive");
	OilUtils::SetNodeState(arrow);
	mRoot->addChild(arrow);
	//物资站
	mMaterialLable = new osg::LOD();
	AddLables("../data/物资站.txt", mMaterialLable, "物资站", osgEarth::Symbology::Color::White, DataType::物资站);
	mapNode->getModelLayerGroup()->addChild(mMaterialLable);
	OilNode->addChild(mMaterialLable);
	//隧道
	mSuidaoLable = new osg::LOD();
	AddLables("../data/隧道.txt", mSuidaoLable, "隧道", osgEarth::Symbology::Color::Yellow, DataType::隧道);
	mapNode->getModelLayerGroup()->addChild(mSuidaoLable);
	//拦油点lable
	mLanYouDianLable = new osg::LOD();
	AddLables("../data/拦油点.txt", mLanYouDianLable, "拦油点", osgEarth::Symbology::Color::White, DataType::拦油点);
	mapNode->getModelLayerGroup()->addChild(mLanYouDianLable);
	OilNode->addChild(mLanYouDianLable);
	//穿河点
	mChuangHeDian = new osg::LOD();
	AddLables("../data/穿河点.txt", mChuangHeDian, "穿河点", osgEarth::Symbology::Color::Black, DataType::穿河点);
	mapNode->getModelLayerGroup()->addChild(mChuangHeDian);
	OilNode->addChild(mChuangHeDian);
	//水电站
	mShuiDianZhan = new osg::LOD();
	AddLables("../data/水电站.txt", mShuiDianZhan, "水电站", Symbology::Color::Yellow, DataType::水电站);
	mapNode->getModelLayerGroup()->addChild(mShuiDianZhan);
	OilNode->addChild(mShuiDianZhan);
	//创建开始界面,三干一支走向线数据和主要的地名
	CreateWorkspace();
	//创建流域面
	CreateWaterShed();
	//创建管带控制
	CreateSanGanYiZhi();
	/***加载作业区数据***/
	AddData_anning();
	AddData_qujing();
	AddData_chuxiong();
	AddData_yuxi();
}
void osgFrame::InitCamera()
{
	RECT rect;
	mViewer = new osgViewer::Viewer();
	::GetWindowRect(m_hWnd, &rect);
	osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits();
	osg::ref_ptr<osg::Referenced> windata = new osgViewer::GraphicsWindowWin32::WindowData(m_hWnd);
	traits->x = 0;
	traits->y = 0;
	traits->width = rect.right - rect.left;
	traits->height = rect.bottom - rect.top;
	traits->windowDecoration = false;
	traits->doubleBuffer = true;
	traits->sharedContext = 0;
	traits->inheritedWindowData = windata;
	osg::GraphicsContext *gc = osg::GraphicsContext::createGraphicsContext(traits); //函数内部把traits的值给改变了
	osg::ref_ptr<osg::Camera> camera = mViewer->getCamera();
	camera->addCullCallback(new osgEarth::Util::AutoClipPlaneCullCallback(mapNode));//视点接近地面时，由于默认的近裁剪面near太大，导致看不见靠近摄像机的物体，这时需要重新调节near和far的值（或者near/far的比率）
	camera->setGraphicsContext(gc);
	camera->setClearColor(osg::Vec4f(0.2, 0.2, 0.4, 0.0)); //设置osgviewer背景色（默认为黑色）
	camera->setViewport(new osg::Viewport(0, 0, traits->width, traits->height));
	camera->setProjectionMatrixAsPerspective(90.0f, static_cast<double>(traits->width) / static_cast<double>(traits->height), 1.0, 1000.0);   //透视投影
	mViewer->setCamera(camera);
	mViewer->setSceneData(mRoot);
	//绑定操作事件
	mViewer->setThreadingModel(osgViewer::ViewerBase::ThreadingModel::SingleThreaded);
	mViewer->realize();
}
void osgFrame::InitOSGEarth(){
	//初始化操作器
	manip = new osgEarth::Util::EarthManipulator;
	mViewer->setCameraManipulator(manip);//必须要在setViewpoint前面
	//先设置一个初始视角，解决旋转跳动问题
	osgEarth::Viewpoint vp("", 101.55, 24.06, 0.0, -2.50, -90.0, 2.0e6);
	manip->setViewpoint(vp);
	FlyTo(101.55, 24.06, 1.9e6, 10); //先停留一下
	FlyTo(103, 25, 3e5);   //旋转到固定视点
	// 设置时间,初始化天空;
	osgEarth::DateTime dateTime(2020, 8, 10, 9); //年月日时
	osgEarth::Util::Ephemeris* ephemeris = new osgEarth::Util::Ephemeris;
	osgEarth::Util::SkyNode* m_pSkyNode = osgEarth::Util::SkyNode::create(mapNode);   //需添加引用 #include <osgEarthUtil/ExampleResources>
	m_pSkyNode->setName("SkyNode");
	m_pSkyNode->setEphemeris(ephemeris);
	m_pSkyNode->setDateTime(dateTime);
	m_pSkyNode->attach(mViewer, 0);
	m_pSkyNode->setLighting(true);
	m_pSkyNode->addChild(mapNode);
	mRoot->addChild(m_pSkyNode);
	//添加点击选线事件
	eh = new CEventHandler;
	eh->mViewer = mViewer;
	eh->mapNode = mapNode;
	eh->computeIntersect = OilNode;
	Pipestrap = new osg::Group();
	Pipestrap->setName("管带");
}
void osgFrame::FlyTo(double lon, double lat, double hei, double flyTime)
{
	manip->setViewpoint(osgEarth::Viewpoint("", lon, lat, 0.0, -2.50, -90.0, hei), flyTime); // 最后一个参数为旋转时间
}
void osgFrame::Render(void* ptr)
{
	osgFrame *osg = (osgFrame*)ptr;
	osgViewer::Viewer *viewer = osg->getViewer();
	while (!viewer->done())
	{
		osg->PreFrameUpdate();
		viewer->frame();
		osg->PostFrameUpdate();
	}
	_endthread();
}
void osgFrame::PreFrameUpdate()
{
	ControlViewRange(minXYZ, maxXYZ);
	double height = GetCurrentViewHight();
	for (int i = 0; i < mapNode->getModelLayerGroup()->getNumChildren(); i++)
	{
		osg::ref_ptr<osg::Node> node = mapNode->getModelLayerGroup()->getChild(i);
		string name = node->getName();
		if (name == "管带")
		{
			node->setNodeMask(height > 5e4f && height < 2.5e6f);
		}
		if (name == "流域面")
		{
			node->setNodeMask(height > 1.5e5f && height < 2e6f);
		}
	}
}
void  osgFrame::PostFrameUpdate()
{
}
void osgFrame::AddLables(string iconPath, string filePath, osg::ref_ptr<osg::LOD> lable, string nodeName, int alignment, osgEarth::Symbology::Color textColor)
{
	const SpatialReference* geoSRS = mapNode->getMapSRS()->getGeographicSRS();
	{
		Style style[3];
		style[0] = CreateStyle(iconPath, 0.8, 12, textColor, alignment);
		style[1] = CreateStyle(iconPath, 0.7, 11, textColor, alignment);
		style[2] = CreateStyle(iconPath, 0.6, 10, textColor, alignment);
		ifstream infile;
		infile.open(filePath.data());
		string s;
		while (getline(infile, s))
		{
			string this_name = s;
			if (s.find("_") != string::npos)
			{
				vector<string> name_num;
				stringUtils::SplitString(s, name_num, "_");
				string name = name_num[0];
				string num = name_num[1];
				this_name = name.append("\n").append(num);
			}
			string FeatureName = s;
			getline(infile, s);
			vector<string> v;
			stringUtils::SplitString(s, v, ",");
			double lon = stringUtils::stringToNum<double>(v[0]);
			double lat = stringUtils::stringToNum<double>(v[1]);
			AddLODLable(lable, GeoPoint(geoSRS, lon, lat), FeatureName, FeatureName, style);
		}
		infile.close();
		mRoot->addChild(lable);
		lable->setNodeMask(0);
		lable->setName(nodeName);
		mapNode->getModelLayerGroup()->addChild(lable);
	}
}
void osgFrame::AddLables(string iconPath, string filePath, osg::ref_ptr<osg::LOD> lable, string nodeName, osgEarth::Symbology::Color textColor)
{
	const SpatialReference* geoSRS = mapNode->getMapSRS()->getGeographicSRS();
	{
		Style style[3];
		style[0] = CreateStyle(iconPath, 0.8, 15, textColor, 4);
		style[1] = CreateStyle(iconPath, 0.7, 14, textColor, 4);
		style[2] = CreateStyle(iconPath, 0.6, 13, textColor, 4);
		ifstream infile;
		infile.open(filePath.data());
		string s;
		while (getline(infile, s))
		{
			string this_name = s;
			if (s.find("_") != string::npos)
			{
				vector<string> name_num;
				stringUtils::SplitString(s, name_num, "_");
				string name = name_num[0];
				string num = name_num[1];
				this_name = name.append("\n").append(num);
			}
			string FeatureName = s;
			getline(infile, s);
			vector<string> v;
			stringUtils::SplitString(s, v, ",");
			double lon = stringUtils::stringToNum<double>(v[0]);
			double lat = stringUtils::stringToNum<double>(v[1]);
			AddLODLable(lable, GeoPoint(geoSRS, lon, lat), FeatureName, "1", style);
		}
		infile.close();
		//mRoot->addChild(lable);
		lable->setNodeMask(0);
		lable->setName(nodeName);
		//mapNode->getModelLayerGroup()->addChild(lable);
	}
}
void osgFrame::AddLables(string filePath, osg::ref_ptr<osg::LOD> lable, string nodeName, osgEarth::Symbology::Color textColor, int datatype)
{
	const SpatialReference* geoSRS = mapNode->getMapSRS()->getGeographicSRS();
	{
		Style **style = GetStyle(datatype, textColor);
		ifstream infile;
		infile.open(filePath.data());
		string s;
		while (getline(infile, s))
		{
			string this_name = s;
			string this_type = s;
			string FeatureName = "";
			string FeatureID = "";
			switch (datatype)
			{
			case DataType::漏油线终点:
			{
				if (s.find("_") != string::npos)
				{
					vector<string> name_type;
					stringUtils::SplitString(s, name_type, ",");
					FeatureName = "";
					FeatureID = name_type[0];
					this_type = name_type[1];
				}
				break;
			}
			case DataType::隧道:
			{
				if (s.find("_") != string::npos)
				{
					vector<string> name_type;
					stringUtils::SplitString(s, name_type, "_");
					vector<string> name_zhunghao;
					stringUtils::SplitString(name_type[0], name_zhunghao, ",");
					if (name_zhunghao.size() > 1)
					{
						FeatureName.append(name_zhunghao[1]);
						FeatureName.append("\n");
						FeatureName.append(name_zhunghao[0]);
					}
					else
					{
						s = name_type[0];
					}
					this_type = name_type[1];
					FeatureID = this_type;
				}
				break;
			}
			case DataType::穿河点:
			{
				if (s.find(",") != string::npos)
				{
					vector<string> name_type;
					stringUtils::SplitString(s, name_type, ",");
					this_type = name_type[0];
					FeatureName = name_type[1];
					FeatureID.append(this_type);
					FeatureID.append(FeatureName);//例如Y1、AYM2
				}
				break;
			}
			case DataType::高后果区:
			{
				if (s.find(",") != string::npos)
				{
					vector<string> name_type;
					stringUtils::SplitString(s, name_type, ",");
					FeatureName = name_type[0];
					FeatureID = name_type[0];
					this_type = stringUtils::string_To_UTF8(name_type[1]);
				}
				break;
			}
			case DataType::物资站:
			{
				if (s.find(",") != string::npos)
				{
					vector<string> name_type;
					stringUtils::SplitString(s, name_type, ",");
					FeatureID = name_type[1];
					FeatureName = name_type[1];
					this_type = name_type[2];
				}
				break;
			}
			case DataType::拦油点:
			{
				if (s.find(",") != string::npos)
				{
					vector<string> name_type;
					stringUtils::SplitString(s, name_type, ",");
					FeatureID = name_type[0];
					vector<string> name_num;
					stringUtils::SplitString(name_type[1], name_num, "_");
					FeatureName = name_num[0] + "\n" + name_num[1];
					this_type = stringUtils::string_To_UTF8("拦油点");
				}
				break;
			}
			case DataType::水电站:
			{
				if (s.find(",") != string::npos)
				{
					vector<string> id_name;
					stringUtils::SplitString(s, id_name, ",");
					FeatureID = id_name[0];
					//FeatureName = id_name[1];
					FeatureName = "";
					this_type = stringUtils::string_To_UTF8("水电站");
				}
				break;
			}
			default:
				break;
			}
			//获取坐标
			getline(infile, s);
			vector<string> v;
			stringUtils::SplitString(s, v, ",");
			double lon = stringUtils::stringToNum<double>(v[0]);
			double lat = stringUtils::stringToNum<double>(v[1]);
			if (datatype == DataType::物资站)
			{
				lat += 0.001;//将物资站图标统一向北偏移
			}
			vector<string> types = GetType(datatype);
			for (int i = 0; i < types.size(); i++)
			{
				//这一块已经封装好，不要改，要改在前面改
				if (stringUtils::UTF8_To_string(this_type) == types[i])
				{
					AddLODLable(lable, GeoPoint(geoSRS, lon, lat), FeatureName, FeatureID, style[i]);
					break;
				}
			}
		}
		infile.close();
		lable->setNodeMask(0);
		lable->setName(nodeName);
	}
}
Style **osgFrame::GetStyle(int type, osgEarth::Symbology::Color textColor)
{
	Style **styles;
	switch (type)
	{
	case DataType::漏油线终点://漏油线终点
	{
		styles = new Style*[5];
		string iconPath[5];
		iconPath[0] = "../Image/shuiku.png";
		iconPath[1] = "../Image/chitang.png";
		iconPath[2] = "../Image/heliu.png";
		iconPath[3] = "../Image/wadi.png";
		iconPath[4] = "../Image/teshu.png";
		for (int i = 0; i < 5; i++)
		{
			styles[i] = new Style[3];
			styles[i][0] = CreateStyle(iconPath[i], 0.5, 5, textColor, 5);
			styles[i][1] = CreateStyle(iconPath[i], 0.4, 4, textColor, 5);
			styles[i][2] = CreateStyle(iconPath[i], 0.3, 3, textColor, 5);
		}
		break;
	}
	case DataType::隧道://隧道
	{
		styles = new Style*[7];
		string iconPath[7];
		float icoSize1 = 0.5;
		float icoSize2 = 0.4;
		float icoSize3 = 0.3;
		iconPath[0] = "../Image/suidao_C.png";
		iconPath[1] = "../Image/suidao_Y.png";
		iconPath[2] = "../Image/suidao_T.png";
		iconPath[3] = "../Image/suidao_CY.png";
		iconPath[4] = "../Image/suidao_CT.png";
		iconPath[5] = "../Image/suidao_YT.png";
		iconPath[6] = "../Image/suidao_CYT.png";
		for (int i = 0; i < 7; i++)
		{
			styles[i] = new Style[3];
			styles[i][0] = CreateStyle(iconPath[i], icoSize1, 13, textColor, 5);
			styles[i][1] = CreateStyle(iconPath[i], icoSize2, 12, textColor, 5);
			styles[i][2] = CreateStyle(iconPath[i], icoSize3, 11, textColor, 5);
		}
		break;
	}
	case DataType::穿河点://穿河点
	{
		styles = new Style*[5];
		TextSymbol::Alignment alignment = TextSymbol::Alignment::ALIGN_CENTER_CENTER;
		styles[0] = new Style[3];
		styles[0][0] = CreateStyle("../Image/chuanhedian_C.png", 0.9, 15, osgEarth::Symbology::Color::Yellow, alignment);
		styles[0][1] = CreateStyle("../Image/chuanhedian_C.png", 0.8, 14, osgEarth::Symbology::Color::Yellow, alignment);
		styles[0][2] = CreateStyle("../Image/chuanhedian_C.png", 0.7, 13, osgEarth::Symbology::Color::Yellow, alignment);
		styles[1] = new Style[3];
		styles[1][0] = CreateStyle("../Image/chuanhedian_C.png", 0.9, 15, osgEarth::Symbology::Color::Yellow, alignment);
		styles[1][1] = CreateStyle("../Image/chuanhedian_C.png", 0.8, 14, osgEarth::Symbology::Color::Yellow, alignment);
		styles[1][2] = CreateStyle("../Image/chuanhedian_C.png", 0.7, 13, osgEarth::Symbology::Color::Yellow, alignment);
		styles[2] = new Style[3];
		styles[2][0] = CreateStyle("../Image/chuanhedian_C.png", 0.9, 15, osgEarth::Symbology::Color::Yellow, alignment);
		styles[2][1] = CreateStyle("../Image/chuanhedian_C.png", 0.8, 14, osgEarth::Symbology::Color::Yellow, alignment);
		styles[2][2] = CreateStyle("../Image/chuanhedian_C.png", 0.7, 13, osgEarth::Symbology::Color::Yellow, alignment);
		styles[3] = new Style[3];
		styles[3][0] = CreateStyle("../Image/chuanhedian_C.png", 0.9, 15, osgEarth::Symbology::Color::Yellow, alignment);
		styles[3][1] = CreateStyle("../Image/chuanhedian_C.png", 0.8, 14, osgEarth::Symbology::Color::Yellow, alignment);
		styles[3][2] = CreateStyle("../Image/chuanhedian_C.png", 0.7, 13, osgEarth::Symbology::Color::Yellow, alignment);
		styles[4] = new Style[3];
		styles[4][0] = CreateStyle("../Image/chuanhedian_Y.png", 0.9, 15, osgEarth::Symbology::Color::Black, alignment);
		styles[4][1] = CreateStyle("../Image/chuanhedian_Y.png", 0.8, 14, osgEarth::Symbology::Color::Black, alignment);
		styles[4][2] = CreateStyle("../Image/chuanhedian_Y.png", 0.7, 13, osgEarth::Symbology::Color::Black, alignment);
		break;
	}
	case DataType::高后果区:
	{
		styles = new Style*[3];
		osgEarth::Symbology::Color color = osgEarth::Symbology::Color::Black;
		TextSymbol::Alignment alignment = TextSymbol::Alignment::ALIGN_CENTER_CENTER;
		styles[0] = new Style[3];
		styles[0][0] = CreateStyle("../Image/dangerArea1.png", 1.2, 15, color, alignment);
		styles[0][1] = CreateStyle("../Image/dangerArea1.png", 1.1, 14, color, alignment);
		styles[0][2] = CreateStyle("../Image/dangerArea1.png", 1, 13, color, alignment);
		styles[1] = new Style[3];
		styles[1][0] = CreateStyle("../Image/dangerArea2.png", 1.2, 15, color, alignment);
		styles[1][1] = CreateStyle("../Image/dangerArea2.png", 1.1, 14, color, alignment);
		styles[1][2] = CreateStyle("../Image/dangerArea2.png", 1, 13, color, alignment);
		styles[2] = new Style[3];
		styles[2][0] = CreateStyle("../Image/dangerArea3.png", 1.2, 15, color, alignment);
		styles[2][1] = CreateStyle("../Image/dangerArea3.png", 1.1, 14, color, alignment);
		styles[2][2] = CreateStyle("../Image/dangerArea3.png", 1, 13, color, alignment);
		break;
	}
	case DataType::物资站:
	{
		styles = new Style*[9];
		osgEarth::Symbology::Color color = osgEarth::Symbology::Color::White;
		TextSymbol::Alignment alignment = TextSymbol::Alignment::ALIGN_CENTER_BOTTOM;
		float icoSize1 = 0.8;
		float icoSize2 = 0.7;
		float icoSize3 = 0.6;
		float textSize1 = 15;
		float textSize2 = 14;
		float textSize3 = 13;
		styles[0] = new Style[3];
		styles[0][0] = CreateStyle("../Image/天然气分输站.png", icoSize1, textSize1, color, alignment);
		styles[0][1] = CreateStyle("../Image/天然气分输站.png", icoSize2, textSize2, color, alignment);
		styles[0][2] = CreateStyle("../Image/天然气分输站.png", icoSize3, textSize3, color, alignment);
		styles[1] = new Style[3];
		styles[1][0] = CreateStyle("../Image/天然气清管站.png", icoSize1, textSize1, color, alignment);
		styles[1][1] = CreateStyle("../Image/天然气清管站.png", icoSize2, textSize2, color, alignment);
		styles[1][2] = CreateStyle("../Image/天然气清管站.png", icoSize3, textSize3, color, alignment);
		styles[2] = new Style[3];
		styles[2][0] = CreateStyle("../Image/成品油输油站.png", icoSize1, textSize1, color, alignment);
		styles[2][1] = CreateStyle("../Image/成品油输油站.png", icoSize2, textSize2, color, alignment);
		styles[2][2] = CreateStyle("../Image/成品油输油站.png", icoSize3, textSize3, color, alignment);
		styles[3] = new Style[3];
		styles[3][0] = CreateStyle("../Image/天然气末站.png", icoSize1, textSize1, color, alignment);
		styles[3][1] = CreateStyle("../Image/天然气末站.png", icoSize2, textSize2, color, alignment);
		styles[3][2] = CreateStyle("../Image/天然气末站.png", icoSize3, textSize3, color, alignment);
		styles[4] = new Style[3];
		styles[4][0] = CreateStyle("../Image/天然气压气站.png", icoSize1, textSize1, color, alignment);
		styles[4][1] = CreateStyle("../Image/天然气压气站.png", icoSize2, textSize2, color, alignment);
		styles[4][2] = CreateStyle("../Image/天然气压气站.png", icoSize3, textSize3, color, alignment);
		styles[5] = new Style[3];
		styles[5][0] = CreateStyle("../Image/保护站4.png", icoSize1, textSize1, color, alignment);
		styles[5][1] = CreateStyle("../Image/保护站4.png", icoSize2, textSize2, color, alignment);
		styles[5][2] = CreateStyle("../Image/保护站4.png", icoSize3, textSize3, color, alignment);
		styles[6] = new Style[3];
		styles[6][0] = CreateStyle("../Image/原油输油站.png", icoSize1, textSize1, color, alignment);
		styles[6][1] = CreateStyle("../Image/原油输油站.png", icoSize2, textSize2, color, alignment);
		styles[6][2] = CreateStyle("../Image/原油输油站.png", icoSize3, textSize3, color, alignment);
		styles[7] = new Style[3];
		styles[7][0] = CreateStyle("../Image/成品油分输站.png", icoSize1, textSize1, color, alignment);
		styles[7][1] = CreateStyle("../Image/成品油分输站.png", icoSize2, textSize2, color, alignment);
		styles[7][2] = CreateStyle("../Image/成品油分输站.png", icoSize3, textSize3, color, alignment);
		styles[8] = new Style[3];
		styles[8][0] = CreateStyle("../Image/炼油厂.png", icoSize1, textSize1, color, alignment);
		styles[8][1] = CreateStyle("../Image/炼油厂.png", icoSize2, textSize2, color, alignment);
		styles[8][2] = CreateStyle("../Image/炼油厂.png", icoSize3, textSize3, color, alignment);
		break;
	}
	case DataType::拦油点:
	{
		styles = new Style*[1];
		string iconPath = "../Image/拦油点2.png";
		osgEarth::Symbology::Color color = osgEarth::Symbology::Color::Black;
		TextSymbol::Alignment alignment = TextSymbol::Alignment::ALIGN_CENTER_CENTER;
		styles[0] = new Style[3];
		styles[0][0] = CreateStyle(iconPath, 0.9, 11, color, alignment);
		styles[0][1] = CreateStyle(iconPath, 0.8, 10, color, alignment);
		styles[0][2] = CreateStyle(iconPath, 0.7, 9, color, alignment);
		break;
	}
	case DataType::水电站:
	{
		styles = new Style*[1];
		string iconPath = "../Image/shuidianzhan1.png";
		osgEarth::Symbology::Color color = osgEarth::Symbology::Color::Black;
		TextSymbol::Alignment alignment = TextSymbol::Alignment::ALIGN_CENTER_CENTER;
		styles[0] = new Style[3];
		styles[0][0] = CreateStyle(iconPath, 0.6, 15, color, alignment);
		styles[0][1] = CreateStyle(iconPath, 0.5, 14, color, alignment);
		styles[0][2] = CreateStyle(iconPath, 0.4, 13, color, alignment);
		break;
	}
	default:
		break;
	}
	return styles;
}
std::vector<std::string> osgFrame::GetType(int type)
{
	std::vector<std::string> types;
	switch (type)
	{
	case DataType::漏油线终点:
	{
		types.push_back("河流");
		types.push_back("池塘");
		types.push_back("水库");
		types.push_back("洼地");
		types.push_back("特殊");
		break;
	}
	case DataType::隧道:
	{
		types.push_back("C");
		types.push_back("Y");
		types.push_back("T");
		types.push_back("C,Y");
		types.push_back("C,T");
		types.push_back("Y,T");
		types.push_back("C,Y,T");
		break;
	}
	case DataType::穿河点:
	{
		types.push_back("ACB");
		types.push_back("AYM");
		types.push_back("AKQ");
		types.push_back("KZ");
		types.push_back("Y");
		break;
	}
	case DataType::高后果区:
	{
		types.push_back("1");
		types.push_back("2");
		types.push_back("3");
		break;
	}
	case DataType::物资站:
	{
		types.push_back("天然气分输站");
		types.push_back("天然气清管站");
		types.push_back("成品油输油站");
		types.push_back("天然气末站");
		types.push_back("天然气压气站");
		types.push_back("保护站");
		types.push_back("原油输油站");
		types.push_back("成品油分输站");
		types.push_back("炼油厂");
		break;
	}
	case DataType::拦油点:
	{
		types.push_back("拦油点");
		break;
	}
	case DataType::水电站:
	{
		types.push_back("水电站");
		break;
	}
	default:
		break;
	}
	return types;
}
Style osgFrame::CreateStyle(string iconPath, float iconSize, float textSize, osgEarth::Symbology::Color textColor, int alignment)
{
	Style pm;
	if (iconPath != "")
	{
		pm.getOrCreate<IconSymbol>()->url()->setLiteral(iconPath);//指定标注图片路径
		pm.getOrCreate<IconSymbol>()->occlusionCull();//遮挡剔除
		pm.getOrCreate<IconSymbol>()->scale() = iconSize;
	}
	pm.getOrCreate<TextSymbol>()->font() = "../data/simhei.ttf";//指定中文字体路径
	pm.getOrCreate<TextSymbol>()->encoding() = osgEarth::Symbology::TextSymbol::ENCODING_UTF8;
	pm.getOrCreate<TextSymbol>()->alignment() = TextSymbol::Alignment(alignment);
	pm.getOrCreate<TextSymbol>()->fill()->color() = textColor;
	pm.getOrCreate<TextSymbol>()->size() = textSize;
	pm.getOrCreate<TextSymbol>()->occlusionCull() = true;//遮挡剔除
	return pm;
}
void osgFrame::AddLODLable(osg::ref_ptr<osg::LOD> lable, GeoPoint geoPoint, string name, Style style[])
{
	osg::ref_ptr<PlaceNode> var1 = new PlaceNode(mapNode, geoPoint, name, style[0]);
	var1->setName(name);
	lable->addChild(var1, 0, 10000);
	osg::ref_ptr<PlaceNode> var2 = new PlaceNode(mapNode, geoPoint, name, style[1]);
	var2->setName(name);
	lable->addChild(var2, 10000, 20000);
	osg::ref_ptr<PlaceNode> var3 = new PlaceNode(mapNode, geoPoint, name, style[2]);
	var3->setName(name);
	lable->addChild(var3, 20000, 400000);
}
void osgFrame::AddLODLable(osg::ref_ptr<osg::LOD> lable, GeoPoint geoPoint, string name, string id, Style style[])
{
	osg::ref_ptr<PlaceNode> var1 = new PlaceNode(mapNode, geoPoint, name, style[0]);
	var1->setName(id);
	lable->addChild(var1, 0, 10000);
	osg::ref_ptr<PlaceNode> var2 = new PlaceNode(mapNode, geoPoint, name, style[1]);
	var2->setName(id);
	lable->addChild(var2, 10000, 20000);
	osg::ref_ptr<PlaceNode> var3 = new PlaceNode(mapNode, geoPoint, name, style[2]);
	var3->setName(id);
	lable->addChild(var3, 20000, 400000);
}
void osgFrame::AddData_anning()
{
	//fengxianfenji_linesTrip_anning = new osg::LOD;
	////加载流域面ive数据
	//osg::ref_ptr<osg::Node> linesTrip_anning = osgDB::readNodeFile("../ive/fengxianfenji/安宁(线框).ive");
	//OilUtils::SetNodeState(linesTrip_anning);
	//fengxianfenji_linesTrip_anning->addChild(linesTrip_anning, 2e4f, 2e5);
	//mRoot->addChild(fengxianfenji_linesTrip_anning);
	AddFengxianfenji("../ive/fengxianfenji/安宁红色(0.4).ive", "安宁作业区高风险区", fengxianfenji_anning_red);
	AddFengxianfenji("../ive/fengxianfenji/安宁黄色(0.4).ive", "安宁作业区较高风险区", fengxianfenji_anning_yellow);
	AddOilPolygon("../ive/polygons/面转线_安宁.ive", "安宁作业区漏油面", OilPolygon_anning);
	AddOillines("../ive/oillines/安宁漏油线.ive", "安宁作业区漏油线", oillines_anning);
	AddmOilLineDestination("../data/point/流向终点_安宁作业区.txt", "安宁作业区漏油线终点", mOilLineDestination_anning);
	AddDangerAreas("../ive/dangerAreas/高后果区-安宁.ive", "安宁作业区高后果区ive", DangerAreas_anning);
	AddDangerAreasLable("../data/gaohouguoqu/高后果区-安宁.txt", "安宁作业区高后果区", DangerAreasLable_anning);
	AddmQuDuanZhang("../data/quduanzhang/quduanzhang_KZ.txt", "安宁作业区区段长", mQuDuanZhang_KZ);
	AddmZhuangHao("../data/zhuanghao/zhuanghao_KZ.txt", "安宁作业区管道走向桩号", mZhuangHao_KZ);
	mZhuangHao_anning = new osg::LOD();
	AddFengxianfenjiZhuangHao("../data/fenjizhuanghao/安宁管带桩号.txt", "安宁作业区风险分级桩号", mZhuangHao_anning);
	//AddShuixi_line("../ive/shuixi/shuixi_anning_line.ive", "安宁作业区水系(线)", shuixi_anning_line);
	//AddShuixi_polygon("../ive/shuixi/shuixi_anning_polygon.ive", "安宁作业区水系(面)", shuixi_anning_polygon);
}
void osgFrame::AddData_qujing()
{
	//fengxianfenji_linesTrip_qujing = new osg::LOD;
	////加载流域面ive数据
	//osg::ref_ptr<osg::Node> linesTrip = osgDB::readNodeFile("../ive/fengxianfenji/曲靖(线框).ive");
	//OilUtils::SetNodeState(linesTrip);
	//fengxianfenji_linesTrip_qujing->addChild(linesTrip, 2e4f, 2e5);
	//mRoot->addChild(fengxianfenji_linesTrip_qujing);
	AddFengxianfenji("../ive/fengxianfenji/曲靖红色(0.4).ive", "曲靖作业区高风险区", fengxianfenji_qujing_red);
	AddFengxianfenji("../ive/fengxianfenji/曲靖黄色(0.4).ive", "曲靖作业区较高风险区", fengxianfenji_qujing_yellow);
	/*Pipestrap->addChild(fengxianfenji_qujing_red);
	Pipestrap->addChild(fengxianfenji_qujing_yellow);*/
	AddOilPolygon("../ive/polygons/面转线_曲靖.ive", "曲靖作业区漏油面", OilPolygon_qujing);
	AddOillines("../ive/oillines/曲靖漏油线.ive", "曲靖作业区漏油线", oillines_qujing);
	AddmOilLineDestination("../data/point/流向终点_曲靖作业区.txt", "曲靖作业区漏油线终点", mOilLineDestination_qujing);
	AddDangerAreas("../ive/dangerAreas/高后果区-曲靖.ive", "曲靖作业区高后果区ive", DangerAreas_qujing);
	AddDangerAreasLable("../data/gaohouguoqu/高后果区-曲靖.txt", "曲靖作业区高后果区", DangerAreasLable_qujing);
	AddmQuDuanZhang("../data/quduanzhang/quduanzhang_AKQ.txt", "曲靖作业区区段长", mQuDuanZhang_AKQ);
	AddmZhuangHao("../data/zhuanghao/zhuanghao_AKQ.txt", "曲靖作业区管道走向桩号", mZhuangHao_AKQ);
	mZhuangHao_qujing = new osg::LOD();
	AddFengxianfenjiZhuangHao("../data/fenjizhuanghao/曲靖管带桩号.txt", "曲靖作业区风险分级桩号", mZhuangHao_qujing);
	//AddShuixi_line("../ive/shuixi/shuixi_qujing_line.ive", "曲靖作业区水系(线)", shuixi_qujing_line);
	//AddShuixi_polygon("../ive/shuixi/shuixi_qujing_polygon.ive", "曲靖作业区水系(面)", shuixi_qujing_polygon);
}
void osgFrame::AddData_chuxiong()
{
	//fengxianfenji_linesTrip_chuxiong = new osg::LOD;
	////加载流域面ive数据
	//osg::ref_ptr<osg::Node> linesTrip = osgDB::readNodeFile("../ive/fengxianfenji/楚雄(线框).ive");
	//OilUtils::SetNodeState(linesTrip);
	//fengxianfenji_linesTrip_chuxiong->addChild(linesTrip, 2e4f, 2e5);
	//mRoot->addChild(fengxianfenji_linesTrip_chuxiong);
	AddFengxianfenji("../ive/fengxianfenji/楚雄红色(0.4).ive", "楚雄作业区高风险区", fengxianfenji_chuxiong_red);
	AddFengxianfenji("../ive/fengxianfenji/楚雄黄色(0.4).ive", "楚雄作业区较高风险区", fengxianfenji_chuxiong_yellow);
	/*Pipestrap->addChild(fengxianfenji_chuxiong_red);
	Pipestrap->addChild(fengxianfenji_chuxiong_yellow);*/
	AddOilPolygon("../ive/polygons/面转线_楚雄.ive", "楚雄作业区漏油面", OilPolygon_chuxiong);
	AddOillines("../ive/oillines/楚雄漏油线.ive", "楚雄作业区漏油线", oillines_chuxiong);
	AddmOilLineDestination("../data/point/流向终点_楚雄作业区.txt", "楚雄作业区漏油线终点", mOilLineDestination_chuxiong);
	AddDangerAreas("../ive/dangerAreas/高后果区-楚雄.ive", "楚雄作业区高后果区ive", DangerAreas_chuxiong);
	AddDangerAreasLable("../data/gaohouguoqu/高后果区-楚雄.txt", "楚雄作业区高后果区", DangerAreasLable_chuxiong);
	AddmQuDuanZhang("../data/quduanzhang/quduanzhang_ACB.txt", "楚雄作业区区段长", mQuDuanZhang_ACB);
	AddmZhuangHao("../data/zhuanghao/zhuanghao_ACB.txt", "楚雄作业区管道走向桩号", mQuDuanZhang_ACB);
	mZhuangHao_chuxiong = new osg::LOD();
	AddFengxianfenjiZhuangHao("../data/fenjizhuanghao/楚雄管带桩号.txt", "楚雄作业区风险分级桩号", mZhuangHao_chuxiong);
	//AddShuixi_line("../ive/shuixi/shuixi_chuxiong_line.ive", "楚雄作业区水系(线)", shuixi_chuxiong_line);
	//AddShuixi_polygon("../ive/shuixi/shuixi_chuxiong_polygon.ive", "楚雄作业区水系(面)", shuixi_chuxiong_polygon);
}
void osgFrame::AddData_yuxi()
{
	//fengxianfenji_linesTrip_yuxi = new osg::LOD;
	////加载流域面ive数据
	//osg::ref_ptr<osg::Node> linesTrip = osgDB::readNodeFile("../ive/fengxianfenji/玉溪(线框).ive");
	//OilUtils::SetNodeState(linesTrip);
	//fengxianfenji_linesTrip_yuxi->addChild(linesTrip, 2e4f, 2e5);
	//mRoot->addChild(fengxianfenji_linesTrip_yuxi);
	AddFengxianfenji("../ive/fengxianfenji/玉溪红色(0.4).ive", "玉溪作业区高风险区", fengxianfenji_yuxi_red);
	AddFengxianfenji("../ive/fengxianfenji/玉溪黄色(0.4).ive", "玉溪作业区较高风险区", fengxianfenji_yuxi_yellow);
	/*Pipestrap->addChild(fengxianfenji_yuxi_red);
	Pipestrap->addChild(fengxianfenji_yuxi_yellow);*/
	AddOilPolygon("../ive/polygons/面转线_玉溪.ive", "玉溪作业区漏油面", OilPolygon_yuxi);
	AddOillines("../ive/oillines/玉溪漏油线.ive", "玉溪作业区漏油线", oillines_yuxi);
	AddmOilLineDestination("../data/point/流向终点_玉溪作业区.txt", "玉溪作业区漏油线终点", mOilLineDestination_yuxi);
	AddDangerAreas("../ive/dangerAreas/高后果区-玉溪.ive", "玉溪作业区高后果区ive", DangerAreas_yuxi);
	AddDangerAreasLable("../data/gaohouguoqu/高后果区-玉溪.txt", "玉溪作业区高后果区", DangerAreasLable_yuxi);
	AddmQuDuanZhang("../data/quduanzhang/quduanzhang_AYM.txt", "玉溪作业区区段长", mQuDuanZhang_AYM);
	AddmZhuangHao("../data/zhuanghao/zhuanghao_AYM.txt", "玉溪作业区管道走向桩号", mZhuangHao_AYM);
	mZhuangHao_yuxi = new osg::LOD();
	AddFengxianfenjiZhuangHao("../data/fenjizhuanghao/玉溪管带桩号.txt", "玉溪作业区风险分级桩号", mZhuangHao_yuxi);
	//AddShuixi_line("../ive/shuixi/shuixi_yuxi_line.ive", "玉溪作业区水系(线)", shuixi_yuxi_line);
	//AddShuixi_polygon("../ive/shuixi/shuixi_yuxi_polygon.ive", "玉溪作业区水系(面)", shuixi_yuxi_polygon);
}
//添加风险分级
void osgFrame::AddFengxianfenji(string path, string name, osg::ref_ptr<osg::Node> node_fengxianfenji)
{
	node_fengxianfenji = osgDB::readNodeFile(path);
	OilUtils::AddIveNode(name, node_fengxianfenji, mapNode);
	mapNode->getModelLayerGroup()->addChild(node_fengxianfenji);//添加目录树种
	OilNode->addChild(node_fengxianfenji);
	node_fengxianfenji->setNodeMask(1);
}
//添加漏油线
void osgFrame::AddOillines(string path, string name, osg::ref_ptr<osg::Node> node_oillines)
{
	node_oillines = osgDB::readNodeFile(path);
	OilUtils::AddIveNode(name, node_oillines, mapNode);
	mapNode->getModelLayerGroup()->addChild(node_oillines);//添加目录树种
	OilNode->addChild(node_oillines);
}
//添加漏油面
void osgFrame::AddOilPolygon(string path, string name, osg::ref_ptr<osg::Node> node_oillines)
{
	node_oillines = osgDB::readNodeFile(path);
	// 开启深度测试，并设置覆盖标志，深度测试总是通过
	osg::Depth* dep = new osg::Depth;
	dep->setFunction(osg::Depth::Function::ALWAYS);
	//修改Z - buffer, 消除直线被地形遮挡开始
	node_oillines->getOrCreateStateSet()->setAttribute(dep, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
	node_oillines->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
	osg::StateSet* state = node_oillines->getOrCreateStateSet();//设置透明度
	state->setMode(GL_BLEND, osg::StateAttribute::ON);
	// 设置渲染系数，哪个模型要显示在前面，就调大该模型的渲染系数
	node_oillines->getOrCreateStateSet()->setRenderBinDetails(1.0, "RenderBin");
	node_oillines->setName(name);
	node_oillines->setNodeMask(0);
	mapNode->getModelLayerGroup()->addChild(node_oillines);//添加目录树种
	//OilUtils::AddIveNode(name, node_oillines, mapNode);
}
//添加漏油线终点
void osgFrame::AddmOilLineDestination(string path, string name, osg::ref_ptr<osg::LOD> node_mOilLineDestination)
{
	node_mOilLineDestination = new osg::LOD();
	AddLables(path, node_mOilLineDestination, name, osgEarth::Symbology::Color::Red, DataType::漏油线终点);
	mapNode->getModelLayerGroup()->addChild(node_mOilLineDestination);
	OilNode->addChild(node_mOilLineDestination);
}
//添加高后果区
void osgFrame::AddDangerAreas(string path, string name, osg::ref_ptr<osg::Node> node_DangerAreas)
{
	node_DangerAreas = osgDB::readNodeFile(path);
	OilUtils::AddIveNode(node_DangerAreas, mRoot);
}
//高后果区lable
void osgFrame::AddDangerAreasLable(string path, string name, osg::ref_ptr<osg::LOD> node_DangerAreasLable)
{
	node_DangerAreasLable = new osg::LOD();
	AddLables(path, node_DangerAreasLable, name, osgEarth::Symbology::Color::White, DataType::高后果区);
	mapNode->getModelLayerGroup()->addChild(node_DangerAreasLable);
	OilNode->addChild(node_DangerAreasLable);
}
//添加区段长
void osgFrame::AddmQuDuanZhang(string path, string name, osg::ref_ptr<osg::LOD> node_mQuDuanZhang)
{
	node_mQuDuanZhang = new osg::LOD();
	string iconPath = "../Image/quduanzhang.png";
	string filePath = path;
	string nodeName = name;
	int alignment = TextSymbol::Alignment::ALIGN_CENTER_BOTTOM;
	osgEarth::Symbology::Color textColor = osgEarth::Symbology::Color::Red;
	const SpatialReference* geoSRS = mapNode->getMapSRS()->getGeographicSRS();
	{
		Style style[3];
		style[0] = CreateStyle(iconPath, 0.6, 14, textColor, alignment);
		style[1] = CreateStyle(iconPath, 0.5, 13, textColor, alignment);
		style[2] = CreateStyle(iconPath, 0.4, 12, textColor, alignment);
		ifstream infile;
		infile.open(filePath.data());
		string s;
		while (getline(infile, s))
		{
			vector<string> name_num;
			if (s.find(",") != string::npos)
			{
				stringUtils::SplitString(s, name_num, ",");
			}
			string ID = name_num[0];
			string FeatureName = name_num[1];
			getline(infile, s);
			vector<string> v;
			stringUtils::SplitString(s, v, ",");
			double lon = stringUtils::stringToNum<double>(v[0]);
			double lat = stringUtils::stringToNum<double>(v[1]);
			AddLODLable(node_mQuDuanZhang, GeoPoint(geoSRS, lon, lat), FeatureName, ID, style);
		}
		infile.close();
		//mRoot->addChild(lable);
		node_mQuDuanZhang->setNodeMask(0);
		node_mQuDuanZhang->setName(nodeName);
		mapNode->getModelLayerGroup()->addChild(node_mQuDuanZhang);
	}
	//OilNode->addChild(node_mQuDuanZhang);
}
//添加桩号
void osgFrame::AddmZhuangHao(string path, string name, osg::ref_ptr<osg::LOD> node_mZhuangHao)
{
	node_mZhuangHao = new osg::LOD();
	string iconPath = "../Image/placemark64.png";
	string filePath = path;
	string nodeName = name;
	int alignment = TextSymbol::Alignment::ALIGN_CENTER_BOTTOM;
	osgEarth::Symbology::Color textColor = osgEarth::Symbology::Color::White;
	const SpatialReference* geoSRS = mapNode->getMapSRS()->getGeographicSRS();
	{
		Style style[3];
		style[0] = CreateStyle(iconPath, 0.8, 14, textColor, alignment);
		style[1] = CreateStyle(iconPath, 0.7, 13, textColor, alignment);
		style[2] = CreateStyle(iconPath, 0.6, 12, textColor, alignment);
		ifstream infile;
		infile.open(filePath.data());
		string s;
		while (getline(infile, s))
		{
			string ID = s;
			string FeatureName = s;
			getline(infile, s);
			vector<string> v;
			stringUtils::SplitString(s, v, ",");
			double lon = stringUtils::stringToNum<double>(v[0]);
			double lat = stringUtils::stringToNum<double>(v[1]);
			AddLODLable(node_mZhuangHao, GeoPoint(geoSRS, lon, lat), FeatureName, ID, style);
		}
		infile.close();
		//mRoot->addChild(node_mZhuangHao);
		node_mZhuangHao->setNodeMask(0);
		node_mZhuangHao->setName(nodeName);
		mapNode->getModelLayerGroup()->addChild(node_mZhuangHao);
	}
}
//风险分级添加桩号
void osgFrame::AddFengxianfenjiZhuangHao(string path, string name, osg::ref_ptr<osg::LOD> node_mZhuangHao)
{
	//node_mZhuangHao = new osg::LOD();
	string iconPath = "../Image/placemark64.png";
	string filePath = path;
	string nodeName = name;
	int alignment = TextSymbol::Alignment::ALIGN_CENTER_BOTTOM;
	osgEarth::Symbology::Color textColor = osgEarth::Symbology::Color::White;
	const SpatialReference* geoSRS = mapNode->getMapSRS()->getGeographicSRS();
	{
		Style style[3];
		style[0] = CreateStyle(iconPath, 0.8, 14, textColor, alignment);
		style[1] = CreateStyle(iconPath, 0.7, 13, textColor, alignment);
		style[2] = CreateStyle(iconPath, 0.6, 12, textColor, alignment);
		ifstream infile;
		infile.open(filePath.data());
		string s;
		while (getline(infile, s))
		{
			string ID = s;
			string FeatureName = s;
			getline(infile, s);
			vector<string> v;
			stringUtils::SplitString(s, v, ",");
			double lon = stringUtils::stringToNum<double>(v[0]);
			double lat = stringUtils::stringToNum<double>(v[1]);
			AddLODLable(node_mZhuangHao, GeoPoint(geoSRS, lon, lat), FeatureName, ID, style);
		}
		infile.close();
		mRoot->addChild(node_mZhuangHao);
		node_mZhuangHao->setNodeMask(0);
		node_mZhuangHao->setName(nodeName);
		//mapNode->getModelLayerGroup()->addChild(node_mZhuangHao);
	}
}
//添加物资站图标
void osgFrame::AddMaterialLable(string path, string name, osg::ref_ptr<osg::LOD> node_MaterialLable)
{
	node_MaterialLable = new osg::LOD();
	AddLables(path, node_MaterialLable, name, osgEarth::Symbology::Color::Red, DataType::物资站);
	OilNode->addChild(node_MaterialLable);
}
//添加穿越水系总图数据
void osgFrame::CreateWorkspace()
{
	const SpatialReference* geoSRS = mapNode->getMapSRS()->getGeographicSRS();
	river = new osg::Group;
	//添加河流ive数据
	osg::ref_ptr<osg::LOD> node_river = new osg::LOD();
	osg::ref_ptr<osg::Node> node_river1 = osgDB::readNodeFile("../ive/heliu/大河流6.ive");
	OilUtils::SetNodeState(node_river1);
	node_river->addChild(node_river1, 2000, 4.1e5);
	river->addChild(node_river);
	//河流文本
	osg::ref_ptr<osg::LOD> heliu_lable = new osg::LOD();
	//osgEarth::Symbology::Color textColor = osgEarth::Symbology::Color(30.0 / 255, 144.0 / 255, 255.0 / 255, 1);
	osgEarth::Symbology::Color textColor = osgEarth::Symbology::Color::Red;
	//写个循环读取txt，添加文字信息
	string filePath = "../data/river/大河流6-名称点.txt";
	ifstream infile;
	infile.open(filePath.data());
	string s;
	while (getline(infile, s))
	{
		string FeatureName = s;
		getline(infile, s);
		vector<string> v;
		stringUtils::SplitString(s, v, ",");
		double lon = stringUtils::stringToNum<double>(v[0]);
		double lat = stringUtils::stringToNum<double>(v[1]);
		heliu_lable->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, lon, lat), FeatureName, CreateStyle("", 1, 18, textColor, 4)), 2000, 4.1e5);
	}
	infile.close();
	river->addChild(heliu_lable);
	river->setName("主要水系");
	mapNode->getModelLayerGroup()->addChild(river);
	// 地名
	osg::ref_ptr<osg::LOD> diming1 = new osg::LOD;
	diming1->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 116.333, 40), stringUtils::string_To_UTF8("北京"), CreateStyle("../Image/shoudu.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e6);
	diming1->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 104.06, 30.67), stringUtils::string_To_UTF8("成都"), CreateStyle("../Image/shenghui.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e6);
	diming1->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 102.7228, 25.02538), stringUtils::string_To_UTF8("昆明"), CreateStyle("../Image/shenghui.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e6);
	osg::ref_ptr<osg::LOD> diming2 = new osg::LOD;
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 102.389609867178, 24.9066734143828), stringUtils::string_To_UTF8("安宁"), CreateStyle("../Image/shi.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 102.102436204818, 25.0377272627165), stringUtils::string_To_UTF8("禄丰"), CreateStyle("../Image/xian.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 101.548597006653, 25.0451284106647), stringUtils::string_To_UTF8("楚雄"), CreateStyle("../Image/shi.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 101.279338657155, 25.2113780711987), stringUtils::string_To_UTF8("南华"), CreateStyle("../Image/xian.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 102.529810885828, 25.1954818224484), stringUtils::string_To_UTF8("富民"), CreateStyle("../Image/xian.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 103.099831808936, 25.4978865299215), stringUtils::string_To_UTF8("嵩明"), CreateStyle("../Image/xian.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 103.812770665859, 25.6189950103259), stringUtils::string_To_UTF8("曲靖"), CreateStyle("../Image/shi.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 102.538585673507, 24.6591326016242), stringUtils::string_To_UTF8("晋宁"), CreateStyle("../Image/xian.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 102.48346133984, 24.3012167225822), stringUtils::string_To_UTF8("玉溪"), CreateStyle("../Image/shi.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 102.906507108721, 23.7325725036925), stringUtils::string_To_UTF8("建水"), CreateStyle("../Image/xian.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 103.313591676838, 23.475009582261), stringUtils::string_To_UTF8("蒙自"), CreateStyle("../Image/shi.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 102.375372870103, 24.9155829345203), stringUtils::string_To_UTF8("安宁炼油厂"), CreateStyle("../Image/lianchang.png", 1, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 102.699112240432, 24.8174660766023), stringUtils::string_To_UTF8("滇池"), CreateStyle("../Image/hupo.png", 0.9, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 102.885893012784, 24.529681500726), stringUtils::string_To_UTF8("抚仙湖"), CreateStyle("../Image/hupo.png", 0.9, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 102.92580440145, 25.0984968191446), stringUtils::string_To_UTF8("长水机场"), CreateStyle("../Image/机场32.png", 0.8, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 100.863341645283, 25.3023535291862), stringUtils::string_To_UTF8("德宏方向"), CreateStyle("", 0.8, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diming2->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, 104.536499057073, 25.9361271803907), stringUtils::string_To_UTF8("贵阳方向"), CreateStyle("", 0.8, 18, osgEarth::Symbology::Color::White, 5)), 2000, 5e5);
	diMing = new osg::Group;
	diMing->addChild(diming1);
	diMing->addChild(diming2);
	diMing->setName("地名");
	//mapNode->getModelLayerGroup()->addChild(diMing);
	mRoot->addChild(diMing);
}
//添加流域面及字lable
void osgFrame::CreateWaterShed()
{
	watershed = new osg::Group;
	watershed->setName("流域面");
	//加载流域面ive数据
	osg::ref_ptr<osg::Node> JSJWaterShed = osgDB::readNodeFile("../ive/流域面/JSJ_LiuYu.ive");
	OilUtils::SetNodeState(JSJWaterShed);
	watershed->addChild(JSJWaterShed);
	osg::ref_ptr<osg::Node> YJWaterShed = osgDB::readNodeFile("../ive/流域面/YJ_LiuYu.ive");
	OilUtils::SetNodeState(YJWaterShed);
	watershed->addChild(YJWaterShed);
	osg::ref_ptr<osg::Node> ZJWaterShed = osgDB::readNodeFile("../ive/流域面/ZJ_LiuYu.ive");
	OilUtils::SetNodeState(ZJWaterShed);
	watershed->addChild(ZJWaterShed);
	//加载流域面字数据
	const SpatialReference* geoSRS = mapNode->getMapSRS()->getGeographicSRS();
	osgEarth::Symbology::Color textColor = osgEarth::Symbology::Color::Black;
	string filePath = "../data/流域.txt";
	ifstream infile;
	infile.open(filePath.data());
	string s;
	while (getline(infile, s))
	{
		string FeatureName = s;
		getline(infile, s);
		vector<string> v;
		stringUtils::SplitString(s, v, ",");
		double lon = stringUtils::stringToNum<double>(v[0]);
		double lat = stringUtils::stringToNum<double>(v[1]);
		watershed->addChild(new osgEarth::Annotation::PlaceNode(mapNode, GeoPoint(geoSRS, lon, lat), FeatureName, CreateStyle("", 1, 25, textColor, 4)));
	}
	infile.close();
	mapNode->getModelLayerGroup()->addChild(watershed);
}
//添加三干一支的线路数据
void osgFrame::CreateSanGanYiZhi()
{
	Pipestrap = new osg::Group();
	Pipestrap->setName("管带");
	//安楚保
	osg::ref_ptr<osg::Node> node_ACB = osgDB::readNodeFile("../ive/SanGanYiZhi/安楚保成品油（线宽2）.ive");
	OilUtils::SetNodeState(node_ACB);
	Pipestrap->addChild(node_ACB);
	//安昆曲
	osg::ref_ptr<osg::Node> node_AKQ = osgDB::readNodeFile("../ive/SanGanYiZhi/安昆曲成品油（线宽2）.ive");
	OilUtils::SetNodeState(node_AKQ);
	Pipestrap->addChild(node_AKQ);
	//安玉蒙自
	osg::ref_ptr<osg::Node> node_AYM = osgDB::readNodeFile("../ive/SanGanYiZhi/安玉蒙成品油（线宽2）.ive");
	OilUtils::SetNodeState(node_AYM);
	Pipestrap->addChild(node_AYM);
	//昆明支线
	osg::ref_ptr<osg::Node> node_KZ = osgDB::readNodeFile("../ive/SanGanYiZhi/昆明支线成品油（线宽2）.ive");
	OilUtils::SetNodeState(node_KZ);
	Pipestrap->addChild(node_KZ);
	//天然气
	osg::ref_ptr<osg::Node> node_T = osgDB::readNodeFile("../ive/SanGanYiZhi/天然气（线宽2）.ive");
	OilUtils::SetNodeState(node_T);
	Pipestrap->addChild(node_T);
	//原油
	osg::ref_ptr<osg::Node> node_Y = osgDB::readNodeFile("../ive/SanGanYiZhi/原油（线宽2）.ive");
	OilUtils::SetNodeState(node_Y);
	Pipestrap->addChild(node_Y);
	mapNode->getModelLayerGroup()->addChild(Pipestrap);
}
osgViewer::Viewer *osgFrame::getViewer()
{
	return mViewer;
}
void osgFrame::MoveViewPoint(osg::Vec2 direction)
{
	osgEarth::Util::Viewpoint ppp = GetCurrentViewPoint();
	osgEarth::GeoPoint pgetfocalPoint = ppp.focalPoint().get();
	pgetfocalPoint.x() += direction[0] * GetCurrentViewHight() / 250000;
	pgetfocalPoint.y() += direction[1] * GetCurrentViewHight() / 250000;
	ppp.focalPoint() = pgetfocalPoint;
	SetCurrentViewPoint(ppp);
}
Viewpoint osgFrame::GetCurrentViewPoint()
{
	return manip->getViewpoint();
}
void  osgFrame::SetCurrentViewPoint(osgEarth::Util::Viewpoint viewpoint)
{
	manip->setViewpoint(viewpoint, 0.001);
}
double osgFrame::GetCurrentViewHight()
{
	return manip->getViewpoint().getRange();
}
void osgFrame::ControlViewRange(osg::Vec3 minXYZ, osg::Vec3 maxXYZ)
{
	osgEarth::Viewpoint pViewPoint = manip->getViewpoint();
	osgEarth::GeoPoint Focalpoint = pViewPoint.focalPoint().get();
	double currentX = Focalpoint.x();
	double currentY = Focalpoint.y();
	double currentZ = Focalpoint.z();
	double currentHeading = pViewPoint.getHeading();
	double currentPitch = pViewPoint.getPitch();
	double currentRange = pViewPoint.getRange();
	//如果超出范围，退回来
	if (currentX > maxXYZ.x())
		manip->setViewpoint(osgEarth::Viewpoint("", maxXYZ.x(), currentY, currentZ, currentHeading, currentPitch, currentRange), 0);
	if (currentX < minXYZ.x())
		manip->setViewpoint(osgEarth::Viewpoint("", minXYZ.x(), currentY, currentZ, currentHeading, currentPitch, currentRange), 0);
	if (currentY > maxXYZ.y())
		manip->setViewpoint(osgEarth::Viewpoint("", currentX, maxXYZ.y(), currentZ, currentHeading, currentPitch, currentRange), 0);
	if (currentY < minXYZ.y())
		manip->setViewpoint(osgEarth::Viewpoint("", currentX, minXYZ.y(), currentZ, currentHeading, currentPitch, currentRange), 0);
	//如果超高，则降回来
	if (currentRange < minXYZ.z())
		manip->setViewpoint(osgEarth::Viewpoint("", currentX, currentY, currentZ, currentHeading, currentPitch, minXYZ.z()), 0);
	if (currentRange > maxXYZ.z())
		manip->setViewpoint(osgEarth::Viewpoint("", currentX, currentY, currentZ, currentHeading, currentPitch, maxXYZ.z()), 0);
}
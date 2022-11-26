#pragma once


#ifdef OSG_USE_UTF8_FILENAME
#define OSGDB_CONVERT_UTF8_FILENAME(s) convertUTF8toUTF16(s).c_str()
#else
#define OSGDB_CONVERT_UTF8_FILENAME(s) convertUTF8toUTF16(convertStringFromCurrentCodePageToUTF8(s)).c_str()
#endif

#include <process.h>  //  /* _beginthread, _endthread */
#include <osgViewer/Viewer>
#include <osgDB/ReadFile>
#include <osgViewer/api/win32/GraphicsWindowWin32>
#include <osgGA/TrackballManipulator>
#include <osgGA/KeySwitchMatrixManipulator>
#include <osgGA/NodeTrackerManipulator>
#include <osgGA/FlightManipulator>
#include <osgGA/DriveManipulator>
#include <osgGA/TerrainManipulator>
#include <osgGA/UFOManipulator>
#include <osgGA/AnimationPathManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/ViewerEventHandlers>
#include <osg/DisplaySettings>
#include <osg/Point>
#include <osgEarth/MapNode>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarthUtil/Controls>
#include <osgEarthUtil/LatLongFormatter>
#include <osgEarth/Map>
#include <osgEarth/TileSource>
#include <osgEarth/Registry>
#include <osgEarthUtil/Common>
#include <osgEarthSymbology/Style>
#include <osgEarthDrivers/tms/TMSOptions>
#include <osgEarthDrivers/gdal/GDALOptions>
#include <osgEarthDrivers/feature_ogr/OGRFeatureOptions>
#include <osgEarthDrivers/agglite/AGGLiteOptions>
#include <osgEarthDrivers/model_feature_geom/FeatureGeomModelOptions>
#include <osgEarthUtil/ExampleResources>
#include <osgEarthFeatures/FeatureSource>
#include <osgEarthDrivers/kml/KML>
#include <osg/Depth>
#include <osgEarthDrivers/cache_filesystem/FileSystemCache>
#include "EventHandler.h"
#include <iostream>
#include <vector>
#include "tinyxml.h"
#include "tinystr.h"

using namespace std;
using namespace osgEarth::Util;
using namespace osgEarth;
using namespace osgEarth::Features;
using namespace osgEarth::Drivers;
using namespace osgEarth::Symbology;
using namespace System;
using namespace System::Threading;

class osgFrame
{
public:
	osgFrame(HWND hWnd);
	~osgFrame();

	void InitOSG();		//初始化OSG
	void InitSceneGraph();	//初始化场景
	void InitCamera();		//初始化相机
	void InitOSGEarth();
	void FlyTo(double lon, double lat, double hei, double flyTime = 2);

	void PreFrameUpdate();	//帧渲染之前更新
	void PostFrameUpdate();		//帧渲染之后更新
	static void Render(void *ptr);	//渲染进程
	osgViewer::Viewer *getViewer();

	osg::ref_ptr<osg::DisplaySettings> displaysettings;

	HWND m_hWnd;
	osgViewer::Viewer *mViewer;
	osg::ref_ptr<osg::Group> mRoot;
	osg::ref_ptr<osgEarth::MapNode> mapNode;
	osg::ref_ptr<osgEarth::Util::EarthManipulator> manip; //地球操作模式


	string workspaceType = "";
	CEventHandler *eh;
	osg::ref_ptr<osg::Group> OilNode;//可拾取节点

	osg::ref_ptr<osg::LOD> mSuidaoLable;//隧道
	osg::ref_ptr<osg::LOD> mMaterialLable;//物资点
	osg::ref_ptr<osg::LOD> mLanYouDianLable;//拦油点
	osg::ref_ptr<osg::LOD> mChuangHeDian;//穿河点
	osg::ref_ptr<osg::LOD> mShuiDianZhan;//水电站
	//osg::ref_ptr<osg::Node> node_river1;//河流
	
	//漏油线
	void AddOillines(string path, string name, osg::ref_ptr<osg::Node> node_oillines);
	osg::ref_ptr<osg::Node> oillines_anning;
	osg::ref_ptr<osg::Node> oillines_chuxiong;
	osg::ref_ptr<osg::Node> oillines_yuxi;
	osg::ref_ptr<osg::Node> oillines_qujing;
	//漏油面
	void AddOilPolygon(string path, string name, osg::ref_ptr<osg::Node> node_OilPolygon);
	osg::ref_ptr<osg::Node> OilPolygon_anning;
	osg::ref_ptr<osg::Node> OilPolygon_chuxiong;
	osg::ref_ptr<osg::Node> OilPolygon_qujing;
	osg::ref_ptr<osg::Node> OilPolygon_yuxi;
	//风险分级
	void AddFengxianfenji(string path, string name, osg::ref_ptr<osg::Node> node_fengxianfenji);
	osg::ref_ptr<osg::Node> fengxianfenji_anning_red;
	osg::ref_ptr<osg::Node> fengxianfenji_anning_yellow;
	osg::ref_ptr<osg::LOD> fengxianfenji_linesTrip_anning;
	osg::ref_ptr<osg::Node> fengxianfenji_chuxiong_red;
	osg::ref_ptr<osg::Node> fengxianfenji_chuxiong_yellow;
	osg::ref_ptr<osg::LOD> fengxianfenji_linesTrip_chuxiong;
	osg::ref_ptr<osg::Node> fengxianfenji_yuxi_red;
	osg::ref_ptr<osg::Node> fengxianfenji_yuxi_yellow;
	osg::ref_ptr<osg::LOD> fengxianfenji_linesTrip_yuxi;
	osg::ref_ptr<osg::Node> fengxianfenji_qujing_red;
	osg::ref_ptr<osg::Node> fengxianfenji_qujing_yellow;
	osg::ref_ptr<osg::LOD> fengxianfenji_linesTrip_qujing;
	//风险分级桩号
	void AddFengxianfenjiZhuangHao(string path, string name, osg::ref_ptr<osg::LOD> node_mZhuangHao);
	osg::ref_ptr<osg::LOD> mZhuangHao_anning;
	osg::ref_ptr<osg::LOD> mZhuangHao_chuxiong;
	osg::ref_ptr<osg::LOD> mZhuangHao_yuxi;
	osg::ref_ptr<osg::LOD> mZhuangHao_qujing;
	//高后果区
	void AddDangerAreas(string path, string name, osg::ref_ptr<osg::Node> node_DangerAreas);
	osg::ref_ptr<osg::Node> DangerAreas_anning;
	osg::ref_ptr<osg::Node> DangerAreas_chuxiong;
	osg::ref_ptr<osg::Node> DangerAreas_yuxi;
	osg::ref_ptr<osg::Node> DangerAreas_qujing;
	//高后果区lable
	void AddDangerAreasLable(string path, string name, osg::ref_ptr<osg::LOD> node_DangerAreasLable);
	osg::ref_ptr<osg::LOD> DangerAreasLable_anning;
	osg::ref_ptr<osg::LOD> DangerAreasLable_chuxiong;
	osg::ref_ptr<osg::LOD> DangerAreasLable_yuxi;
	osg::ref_ptr<osg::LOD> DangerAreasLable_qujing;
	//拦油点
	void AddLanYouDianLable(string path, string name, osg::ref_ptr<osg::LOD> node_LanYouDianLable);
	osg::ref_ptr<osg::LOD> LanYouDianLable_anning;
	osg::ref_ptr<osg::LOD> LanYouDianLable_chuxiong;
	osg::ref_ptr<osg::LOD> LanYouDianLable_yuxi;
	osg::ref_ptr<osg::LOD> LanYouDianLable_qujing;
	//物资点
	void AddMaterialLable(string path, string name, osg::ref_ptr<osg::LOD> node_MaterialLable);
	osg::ref_ptr<osg::LOD> MaterialLable_anning;
	osg::ref_ptr<osg::LOD> MaterialLable_chuxiong;
	osg::ref_ptr<osg::LOD> MaterialLable_yuxi;
	osg::ref_ptr<osg::LOD> MaterialLable_qujing;
	//漏油线终点
	void AddmOilLineDestination(string path, string name, osg::ref_ptr<osg::LOD> node_mOilLineDestination);
	osg::ref_ptr<osg::LOD> mOilLineDestination_anning;
	osg::ref_ptr<osg::LOD> mOilLineDestination_chuxiong;
	osg::ref_ptr<osg::LOD> mOilLineDestination_yuxi;
	osg::ref_ptr<osg::LOD> mOilLineDestination_qujing;
	//区段长
	void AddmQuDuanZhang(string path, string name, osg::ref_ptr<osg::LOD> node_mQuDuanZhang);
	osg::ref_ptr<osg::LOD> mQuDuanZhang_KZ;
	osg::ref_ptr<osg::LOD> mQuDuanZhang_AKQ;
	osg::ref_ptr<osg::LOD> mQuDuanZhang_AYM;
	osg::ref_ptr<osg::LOD> mQuDuanZhang_ACB;
	//桩号
	void AddmZhuangHao(string path, string name, osg::ref_ptr<osg::LOD> node_mZhuangHao);
	osg::ref_ptr<osg::LOD> mZhuangHao_ACB;
	osg::ref_ptr<osg::LOD> mZhuangHao_AYM;
	osg::ref_ptr<osg::LOD> mZhuangHao_AKQ;
	osg::ref_ptr<osg::LOD> mZhuangHao_KZ;

	//添加作业区数据
	void AddData_anning();
	void AddData_qujing();
	void AddData_chuxiong();
	void AddData_yuxi();

	Style **GetStyle(int type, osgEarth::Symbology::Color textColor);
	std::vector<std::string> GetType(int type);
	Style CreateStyle(string iconPath, float iconSize, float textSize, osgEarth::Symbology::Color textColor, int alignment);//创建样式
	
	enum DataType;
	void AddLables(string iconPath, string filePath, osg::ref_ptr<osg::LOD> lable, string nodeName, int alignment, osgEarth::Symbology::Color textColor);
	void AddLables(string filePath, osg::ref_ptr<osg::LOD> lable, string name, osgEarth::Symbology::Color textColor, int type);
	void AddLables(string iconPath, string filePath, osg::ref_ptr<osg::LOD> lable, string name, osgEarth::Symbology::Color textColor);
	void AddLODLable(osg::ref_ptr<osg::LOD> lable, GeoPoint geoPoint, string name, Style style[]);//添加LOD模式的Lable
	void AddLODLable(osg::ref_ptr<osg::LOD> lable, GeoPoint geoPoint, string name, string id, Style style[]);//添加LOD模式的Lable
	
	//开始界面
	osg::ref_ptr<osg::Group> workSpaceNode;
	osg::ref_ptr<osg::Group> diMing;
	osg::ref_ptr<osg::Group> river;
	osg::ref_ptr<osg::Node> arrow;
	osg::ref_ptr<osg::Group> watershed;//流域面
	osg::ref_ptr<osg::Group> watershed_ive;
	osg::ref_ptr<osg::Group> watershed_lable;
	osg::ref_ptr<osg::Group> Pipestrap;

	void CreateWorkspace();
	void CreateWaterShed();
	void CreateSanGanYiZhi();

	//控制视点位置不要超出固定范围
	osg::Vec3 minXYZ = osg::Vec3(0, 0, 0);
	osg::Vec3 maxXYZ = osg::Vec3(FLT_MAX, FLT_MAX, 3.86e6);
	void ControlViewRange(osg::Vec3 minXYZ, osg::Vec3 maxXYZ);

	void MoveViewPoint(osg::Vec2 direction);
	osgEarth::Util::Viewpoint GetCurrentViewPoint();
	void  SetCurrentViewPoint(osgEarth::Util::Viewpoint viewpoint);
	double GetCurrentViewHight();
};

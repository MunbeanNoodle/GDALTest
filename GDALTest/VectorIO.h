//=====**********************************=====//
//         Update To: 2016.08.25              //
//         The Version of GDAL is 202         //
//=====**********************************=====//

//============================================//
//						                      //
//   基于GDAL和OGR库对矢量GIS文件的读写IO类   //
//   每个类对象只能同时操作一个文件           //
//						                      //
//============================================//

#pragma once
#include "gdal/include/gdal_priv.h"
#include "gdal/include/ogrsf_frmts.h"
#pragma comment(lib,"gdal/gdal_i.lib")

#include <map>
#include <vector>
using namespace std;

#define MAX_TEXT_CONVERT_MEM	1024

//---文件类型枚举-----------------------------------------------------------------------------------------------
typedef enum en_FILE_TYPE
{
	UNKNOWN_FILE     = -1,	//未知（暂不支持）的文件类型
	ESRI_SHAPE_FILE  = 1,	//*.shp
	MAPINFO_TAB_FILE = 2,	//*.tab
	MAPINFO_MIF_FILE = 3	//*.mif
}FILE_TYPE;
//-----------------------------------------------------------------------------------------------------------------

//---属性表信息结构体-----------------------------------------------------------------------------------------------
typedef struct tag_ATTRIBUTE_INFO
{
	int iFieldNum;						//字段数
	vector<OGRFieldType> enFieldType;	//各字段数据类型
	vector<CString> strFieldName;		//各字段名称
}ATTRIBUTE_INFO;
//-----------------------------------------------------------------------------------------------------------------

//---基本信息结构体-----------------------------------------------------------------------------------------------
typedef struct tag_VECTOR_BASIC_INFO
{
	FILE_TYPE enFileType;						//文件类型
	int iFileStatus;							//文件状态: -1:未打开文件, 0:正常打开, 1:新建成功
	int iLayerNum;								//图层数
	double dBounds[4];							//空间范围
	vector<OGRSpatialReference>  SpatialRef;	//各图层空间参考
	vector<OGRwkbGeometryType> GeometryType;	//各图层图元类型（SHAPE FILE需要）
	vector<INT64> i64FeatureNum;				//各图层的对象数（注意是INT64型变量）	
	vector<CString> strLayerName;				//各图层名称
	vector<ATTRIBUTE_INFO> Attribute;			//各图层属性表表头信息
}VECTOR_BASIC_INFO;
//-----------------------------------------------------------------------------------------------------------------

//---空间对象类型枚举----------------------------------------------------------------------------------------------
typedef enum tag_GEOMETRY_TYPE
{
	UNKNOWN_TYPE    = -1,						//未知类型
	SINGLE_POINT    = wkbPoint,					//单个点
	SINGLE_POLYLINE = wkbLineString,			//单个线
	SINGLE_POLYGON  = wkbPolygon,				//单个多边形
	MULTI_POINTS    = wkbMultiPoint,			//多个点
	MULTI_POLYLINES = wkbMultiLineString,		//多个线
	MULTI_POLYGONS  = wkbMultiPolygon,			//多个多边形
	MULTI_GEOMETRYS = wkbGeometryCollection		//多个点线面
}GEOMETRY_TYPE;
//-----------------------------------------------------------------------------------------------------------------

//---结点坐标结构体------------------------------------------------------------------------------------------------
typedef struct tag_NODE_POINT
{
	double dX;	//X坐标
	double dY;	//Y坐标
}NODE_POINT;
//-----------------------------------------------------------------------------------------------------------------

//---外多边形（多边形的洞:Ring）结构体-----------------------------------------------------------------------------
typedef struct tag_RING
{
	unsigned int uiNodePointNum;		//结点数
	vector<NODE_POINT> NodePointList;	//结点坐标列表
}RING;
//-----------------------------------------------------------------------------------------------------------------

//---基本图元结构体（包含了点、线、多边形(带洞)）------------------------------------------------------------------
typedef struct tag_BASIC_GEOMETRY
{
	GEOMETRY_TYPE GeometryType;			//图元类型
	unsigned int uiNodePointNum;		//结点数
	vector<NODE_POINT> NodePointList;	//节点坐标列表
	unsigned int uiRingNum;				//外多边形数
	vector<RING> Rings;					//外多边形数组
	unsigned int uiSubGeometryNum;		//子图元对象数量
}BASIC_GEOMETRY;
//-----------------------------------------------------------------------------------------------------------------

//---空间对象的空间信息结构体--------------------------------------------------------------------------------------
typedef struct tag_GEO_OBJECT_SPATIAL
{
	BASIC_GEOMETRY MainGeometry;			//父图元对象（主对象）
	vector<BASIC_GEOMETRY> SubGeometrys;	//子图元对象数组
	vector<BASIC_GEOMETRY> PartGeometrys;	//孙图元对象数组
}GEO_OBJECT_SPATIAL;
//-----------------------------------------------------------------------------------------------------------------

//---空间对象的属性信息结构体--------------------------------------------------------------------------------------
typedef struct tag_GEO_OBJECT_ATTRIBUTE
{
	unsigned int uiFieldNum;			//字段数
	vector<CString> FieldInfoString;	//各字段值（以字符串形式表示）
}GEO_OBJECT_ATTRIBUTE;
//-----------------------------------------------------------------------------------------------------------------


//---CVectorIO类实体-----------------------------------------------------------------------------------------------
class CVectorIO
{
public:
	CVectorIO(void);			//构造函数
	~CVectorIO(void);			//析构函数

//私有变量
private:
	CString strFilePath;		//当前打开的图像路径

	CString strFileExt;			//当前打开图像后缀名

	GDALDataset * pDataset;		//文件句柄

	VECTOR_BASIC_INFO BasicInfo;		//基本信息	

	map<CString,CString> Map_SHAPE_OPTION_LIST;		//ShapeFile创建选项列表

	map<CString,CString> Map_TABMIF_OPTION_LIST;	//MapInfo创建选项列表

	vector<CString> strErrMsgs;	//错误信息容器

	BOOL bErrLogOpen;			//错误日志文件打开标记

	CStdioFile ErrLog;			//错误日志文件句柄

	BOOL bDeleteFeature;		//删除空间对象操作标识

	BOOL InitDataVessel();		//初始化数据容器

	INT64 GetFeatureNum(OGRLayer * pLayer);	//遍历获取对象数

	int iCoding;	//在读属性表信息时（字段名+字段值）,强制使用某种编码转换为标准CString, 该标识可在OpenFile中指定. 含义: -1为不转换,0为UTF-8

	wchar_t *  pUnicode;	//为避免频繁申请释放内存设立一个公用空间指针，专门服务与字符编码转换

	CString ConvertTextInCoding(const char * pText, int iCodingCur);	//使用指定编码转换字符串至CString,目前实现了UTF-8的转换

	BOOL GetTextCodingFromFileForShape(CString strName);	//如果有CPG文件，则从CPG文件中获取字符集编码方式

//成员函数
public:
	//获取错误信息
	//参数列表: 是否以MessageBox形式弹出错误信息(TRUE为弹出), 是否已文件形式输出错误信息(TRUE为输出), 注意！错误信息输出文件路径为打开或新建文件路径+_ErrLog.txt,如果该路径为程序当前路径+_ErrLog.txt
	BOOL GetErrorMessage(BOOL bMsgBox, BOOL bExportErrLog);

	//关闭文件. 建议操作完文件后必须调用该函数完成读写操作及释放资源
	BOOL CloseFile();

	//打开文件
	//参数列表: 文件路径
	BOOL OpenFile(CString strFileName, VECTOR_BASIC_INFO & VectorInfo, int iForceCoding = -1);

	//创建新文件
	//参数列表: 文件路径, 基本信息(仅使用图层名称(不过好像没什么用,可以不填,程序自己会处理), 对象类型(创建shp时需要), 属性表, 空间范围(创建MapInfo时需要,xmin,ymin,xmax,ymax)
	BOOL CreateFile(CString strFileName, VECTOR_BASIC_INFO VectorInfo);

	//读取空间对象信息
	//参数列表:空间对象ID（从0开始）, 空间信息容器, 属性信息容器, 是否读取空间信息（TRUE为读取）, 是否读取属性信息（TRUE为读取）, 操作图层ID（从0开始,默认为0,可不指定）
	BOOL ReadGeoObject(INT64 i64FeatureNo, GEO_OBJECT_SPATIAL & GeoObjSpatial, GEO_OBJECT_ATTRIBUTE & GeoObjAttribute, BOOL bReadSpatialInfo, BOOL bReadAttributeInfo, int iLayerNo = 0);

	//写入空间对象信息
	//参数列表:空间对象ID（更新时有用,新建时随便输入）, 空间信息, 属性信息, 是否更新空间信息（TRUE为更新）, 是否更新属性信息（TRUE为更新）, 是否为新建对象（TRUE为新建,FALSE为更新已有的）, 操作图层ID（从0开始,默认为0,可不指定）
	BOOL WriteGeoObject(INT64 i64FeatureNo, GEO_OBJECT_SPATIAL GeoObjSpatial, GEO_OBJECT_ATTRIBUTE GeoObjAttribute, BOOL bUpdateSpatialInfo, BOOL bUpdateAttributeInfo, BOOL bCreateNewFeature, int iLayerNo = 0);

	//删除空间对象
	//参数列表:空间对象ID（从0开始）,操作图层ID（从0开始,默认为0,可不指定）
	BOOL DeleteGeoObject(INT64 i64FeatureNo, int iLayerNo = 0);

};

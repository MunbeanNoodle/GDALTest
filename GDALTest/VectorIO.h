//=====**********************************=====//
//         Update To: 2016.08.25              //
//         The Version of GDAL is 202         //
//=====**********************************=====//

//============================================//
//						                      //
//   ����GDAL��OGR���ʸ��GIS�ļ��Ķ�дIO��   //
//   ÿ�������ֻ��ͬʱ����һ���ļ�           //
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

//---�ļ�����ö��-----------------------------------------------------------------------------------------------
typedef enum en_FILE_TYPE
{
	UNKNOWN_FILE     = -1,	//δ֪���ݲ�֧�֣����ļ�����
	ESRI_SHAPE_FILE  = 1,	//*.shp
	MAPINFO_TAB_FILE = 2,	//*.tab
	MAPINFO_MIF_FILE = 3	//*.mif
}FILE_TYPE;
//-----------------------------------------------------------------------------------------------------------------

//---���Ա���Ϣ�ṹ��-----------------------------------------------------------------------------------------------
typedef struct tag_ATTRIBUTE_INFO
{
	int iFieldNum;						//�ֶ���
	vector<OGRFieldType> enFieldType;	//���ֶ���������
	vector<CString> strFieldName;		//���ֶ�����
}ATTRIBUTE_INFO;
//-----------------------------------------------------------------------------------------------------------------

//---������Ϣ�ṹ��-----------------------------------------------------------------------------------------------
typedef struct tag_VECTOR_BASIC_INFO
{
	FILE_TYPE enFileType;						//�ļ�����
	int iFileStatus;							//�ļ�״̬: -1:δ���ļ�, 0:������, 1:�½��ɹ�
	int iLayerNum;								//ͼ����
	double dBounds[4];							//�ռ䷶Χ
	vector<OGRSpatialReference>  SpatialRef;	//��ͼ��ռ�ο�
	vector<OGRwkbGeometryType> GeometryType;	//��ͼ��ͼԪ���ͣ�SHAPE FILE��Ҫ��
	vector<INT64> i64FeatureNum;				//��ͼ��Ķ�������ע����INT64�ͱ�����	
	vector<CString> strLayerName;				//��ͼ������
	vector<ATTRIBUTE_INFO> Attribute;			//��ͼ�����Ա��ͷ��Ϣ
}VECTOR_BASIC_INFO;
//-----------------------------------------------------------------------------------------------------------------

//---�ռ��������ö��----------------------------------------------------------------------------------------------
typedef enum tag_GEOMETRY_TYPE
{
	UNKNOWN_TYPE    = -1,						//δ֪����
	SINGLE_POINT    = wkbPoint,					//������
	SINGLE_POLYLINE = wkbLineString,			//������
	SINGLE_POLYGON  = wkbPolygon,				//���������
	MULTI_POINTS    = wkbMultiPoint,			//�����
	MULTI_POLYLINES = wkbMultiLineString,		//�����
	MULTI_POLYGONS  = wkbMultiPolygon,			//��������
	MULTI_GEOMETRYS = wkbGeometryCollection		//���������
}GEOMETRY_TYPE;
//-----------------------------------------------------------------------------------------------------------------

//---�������ṹ��------------------------------------------------------------------------------------------------
typedef struct tag_NODE_POINT
{
	double dX;	//X����
	double dY;	//Y����
}NODE_POINT;
//-----------------------------------------------------------------------------------------------------------------

//---�����Σ�����εĶ�:Ring���ṹ��-----------------------------------------------------------------------------
typedef struct tag_RING
{
	unsigned int uiNodePointNum;		//�����
	vector<NODE_POINT> NodePointList;	//��������б�
}RING;
//-----------------------------------------------------------------------------------------------------------------

//---����ͼԪ�ṹ�壨�����˵㡢�ߡ������(����)��------------------------------------------------------------------
typedef struct tag_BASIC_GEOMETRY
{
	GEOMETRY_TYPE GeometryType;			//ͼԪ����
	unsigned int uiNodePointNum;		//�����
	vector<NODE_POINT> NodePointList;	//�ڵ������б�
	unsigned int uiRingNum;				//��������
	vector<RING> Rings;					//����������
	unsigned int uiSubGeometryNum;		//��ͼԪ��������
}BASIC_GEOMETRY;
//-----------------------------------------------------------------------------------------------------------------

//---�ռ����Ŀռ���Ϣ�ṹ��--------------------------------------------------------------------------------------
typedef struct tag_GEO_OBJECT_SPATIAL
{
	BASIC_GEOMETRY MainGeometry;			//��ͼԪ����������
	vector<BASIC_GEOMETRY> SubGeometrys;	//��ͼԪ��������
	vector<BASIC_GEOMETRY> PartGeometrys;	//��ͼԪ��������
}GEO_OBJECT_SPATIAL;
//-----------------------------------------------------------------------------------------------------------------

//---�ռ�����������Ϣ�ṹ��--------------------------------------------------------------------------------------
typedef struct tag_GEO_OBJECT_ATTRIBUTE
{
	unsigned int uiFieldNum;			//�ֶ���
	vector<CString> FieldInfoString;	//���ֶ�ֵ�����ַ�����ʽ��ʾ��
}GEO_OBJECT_ATTRIBUTE;
//-----------------------------------------------------------------------------------------------------------------


//---CVectorIO��ʵ��-----------------------------------------------------------------------------------------------
class CVectorIO
{
public:
	CVectorIO(void);			//���캯��
	~CVectorIO(void);			//��������

//˽�б���
private:
	CString strFilePath;		//��ǰ�򿪵�ͼ��·��

	CString strFileExt;			//��ǰ��ͼ���׺��

	GDALDataset * pDataset;		//�ļ����

	VECTOR_BASIC_INFO BasicInfo;		//������Ϣ	

	map<CString,CString> Map_SHAPE_OPTION_LIST;		//ShapeFile����ѡ���б�

	map<CString,CString> Map_TABMIF_OPTION_LIST;	//MapInfo����ѡ���б�

	vector<CString> strErrMsgs;	//������Ϣ����

	BOOL bErrLogOpen;			//������־�ļ��򿪱��

	CStdioFile ErrLog;			//������־�ļ����

	BOOL bDeleteFeature;		//ɾ���ռ���������ʶ

	BOOL InitDataVessel();		//��ʼ����������

	INT64 GetFeatureNum(OGRLayer * pLayer);	//������ȡ������

	int iCoding;	//�ڶ����Ա���Ϣʱ���ֶ���+�ֶ�ֵ��,ǿ��ʹ��ĳ�ֱ���ת��Ϊ��׼CString, �ñ�ʶ����OpenFile��ָ��. ����: -1Ϊ��ת��,0ΪUTF-8

	wchar_t *  pUnicode;	//Ϊ����Ƶ�������ͷ��ڴ�����һ�����ÿռ�ָ�룬ר�ŷ������ַ�����ת��

	CString ConvertTextInCoding(const char * pText, int iCodingCur);	//ʹ��ָ������ת���ַ�����CString,Ŀǰʵ����UTF-8��ת��

	BOOL GetTextCodingFromFileForShape(CString strName);	//�����CPG�ļ������CPG�ļ��л�ȡ�ַ������뷽ʽ

//��Ա����
public:
	//��ȡ������Ϣ
	//�����б�: �Ƿ���MessageBox��ʽ����������Ϣ(TRUEΪ����), �Ƿ����ļ���ʽ���������Ϣ(TRUEΪ���), ע�⣡������Ϣ����ļ�·��Ϊ�򿪻��½��ļ�·��+_ErrLog.txt,�����·��Ϊ����ǰ·��+_ErrLog.txt
	BOOL GetErrorMessage(BOOL bMsgBox, BOOL bExportErrLog);

	//�ر��ļ�. ����������ļ��������øú�����ɶ�д�������ͷ���Դ
	BOOL CloseFile();

	//���ļ�
	//�����б�: �ļ�·��
	BOOL OpenFile(CString strFileName, VECTOR_BASIC_INFO & VectorInfo, int iForceCoding = -1);

	//�������ļ�
	//�����б�: �ļ�·��, ������Ϣ(��ʹ��ͼ������(��������ûʲô��,���Բ���,�����Լ��ᴦ��), ��������(����shpʱ��Ҫ), ���Ա�, �ռ䷶Χ(����MapInfoʱ��Ҫ,xmin,ymin,xmax,ymax)
	BOOL CreateFile(CString strFileName, VECTOR_BASIC_INFO VectorInfo);

	//��ȡ�ռ������Ϣ
	//�����б�:�ռ����ID����0��ʼ��, �ռ���Ϣ����, ������Ϣ����, �Ƿ��ȡ�ռ���Ϣ��TRUEΪ��ȡ��, �Ƿ��ȡ������Ϣ��TRUEΪ��ȡ��, ����ͼ��ID����0��ʼ,Ĭ��Ϊ0,�ɲ�ָ����
	BOOL ReadGeoObject(INT64 i64FeatureNo, GEO_OBJECT_SPATIAL & GeoObjSpatial, GEO_OBJECT_ATTRIBUTE & GeoObjAttribute, BOOL bReadSpatialInfo, BOOL bReadAttributeInfo, int iLayerNo = 0);

	//д��ռ������Ϣ
	//�����б�:�ռ����ID������ʱ����,�½�ʱ������룩, �ռ���Ϣ, ������Ϣ, �Ƿ���¿ռ���Ϣ��TRUEΪ���£�, �Ƿ����������Ϣ��TRUEΪ���£�, �Ƿ�Ϊ�½�����TRUEΪ�½�,FALSEΪ�������еģ�, ����ͼ��ID����0��ʼ,Ĭ��Ϊ0,�ɲ�ָ����
	BOOL WriteGeoObject(INT64 i64FeatureNo, GEO_OBJECT_SPATIAL GeoObjSpatial, GEO_OBJECT_ATTRIBUTE GeoObjAttribute, BOOL bUpdateSpatialInfo, BOOL bUpdateAttributeInfo, BOOL bCreateNewFeature, int iLayerNo = 0);

	//ɾ���ռ����
	//�����б�:�ռ����ID����0��ʼ��,����ͼ��ID����0��ʼ,Ĭ��Ϊ0,�ɲ�ָ����
	BOOL DeleteGeoObject(INT64 i64FeatureNo, int iLayerNo = 0);

};

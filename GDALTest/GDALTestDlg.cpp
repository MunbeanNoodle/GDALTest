// GDALTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "GDALTest.h"
#include "GDALTestDlg.h"
#include "VectorIO.h"

#include <vector>
using namespace std;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

typedef struct tag_FIELD_INFO
{
	CString strName;
	OGRFieldType Type;
}FIELD_INFO;

typedef struct tag_MY_POINT
{
	double dX;
	double dY;
}MY_POINT;

typedef struct tag_MY_HOLE
{
	int iPtNum;
	vector<MY_POINT> Pts;
}MY_HOLE;

typedef struct tag_MY_POLYGON
{
	//外环
	int iExRingPtNum;
	vector<MY_POINT> ExRingPts;

	//内环
	int iInRingNum;
	vector<MY_HOLE> Holes;		
}MY_POLYGON;


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CGDALTestDlg 对话框




CGDALTestDlg::CGDALTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CGDALTestDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGDALTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CGDALTestDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BN_READ_IMAGE, &CGDALTestDlg::OnBnClickedBnReadImage)
	ON_BN_CLICKED(IDC_BN_READ_VECTOR, &CGDALTestDlg::OnBnClickedBnReadVector)
END_MESSAGE_MAP()


// CGDALTestDlg 消息处理程序

BOOL CGDALTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	GDALAllRegister();

	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");
	CPLSetConfigOption("SHAPE_ENCODING","");


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CGDALTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CGDALTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标显示。
//
HCURSOR CGDALTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CGDALTestDlg::OnBnClickedBnReadImage()
{
	//声明一个数据集（图像句柄）
	GDALDataset * poDataset;	

	//打开图像文件
	poDataset = (GDALDataset *)GDALOpen("D:\\Fusion.tif", GA_ReadOnly);	

	//如果打开失败，则报错
	if(poDataset == NULL){MessageBox("Open file error!");	return;}

	//---获取图像基本信息
	
	//获取图像格式
	CString str = poDataset->GetDriver()->GetDescription();
	
	//获取XY大小
	int iRows = poDataset->GetRasterYSize();
	int iCols = poDataset->GetRasterXSize();
	
	//获取波段数
	int iBands = poDataset->GetRasterCount();
	
	//获取投影字符串
	CString str1 = GDALGetProjectionRef(poDataset);
	
	//获取仿射变换系数
	double dTrans[6];	
	poDataset->GetGeoTransform(dTrans);

	//读波段数据
	BYTE * pData = NULL;
	pData = (BYTE *)malloc(iRows * iCols * 1 * sizeof(BYTE));
	if(pData == NULL){MessageBox("malloc error!"); return;}

	GDALRasterBand * pBand = poDataset->GetRasterBand(3);
	CPLErr Ret = pBand->RasterIO(GF_Read, 0,0,iCols,iRows,pData,iCols,iRows,GDT_Byte,0,0);

	//获取元数据
	char** papszMetadata = NULL;
	papszMetadata = GDALGetMetadata(poDataset, NULL );


	/*CString strLine = "";

	if(CSLCount(papszMetadata) > 0 )
	{
		str = "META_DATA\n";							strLine += str;
		for(int i = 0; papszMetadata[i] != NULL; i++ )
		{
			str.Format("    %s\n", papszMetadata[i]);	strLine += str;
		}
	}

	MessageBox(strLine);*/

	//创建图像
	GDALDriver * pDriver = NULL;
	pDriver = GetGDALDriverManager()->GetDriverByName("GTIFF");

	GDALDataset * poDatasetW = pDriver->Create("D:\\new.tif",  iCols,  iRows,  1, GDT_Byte,  NULL);
	if(poDatasetW == NULL){MessageBox("Create file error！"); return;}

	GDALRasterBand * pBandNew = poDatasetW->GetRasterBand(1);

	pBandNew->RasterIO(GF_Write,0,0,iCols,iRows,pData,iCols,iRows,GDT_Byte,0,0);

	GDALClose(poDataset);
	GDALClose(poDatasetW);

	MessageBox("Done!");
	int x=0;

	CRasterIO1 MyImgIO;
	MyImgIO.OpenImage("aaa");
}

void CGDALTestDlg::OnBnClickedBnReadVector()
{
	//打开矢量文件
	GDALDataset * pDataset;
	
	pDataset = (GDALDataset *)GDALOpenEx("D:\\zj_Polygon.shp", GDAL_OF_VECTOR, NULL, NULL, NULL);

	//获取图层数量
	int LayerNum = pDataset->GetLayerCount();

	//获取图层句柄,下标从0开始
	OGRLayer * pLayer = pDataset->GetLayer(0);

	//获取图层空间范围
	OGREnvelope Envelope;
	pLayer->GetExtent(&Envelope);

	//获取图层名称
	CString str =pLayer->GetName();
	
	//获取图层空间参考
	OGRSpatialReference * SpatialReference = pLayer->GetSpatialRef();

	//获取图层中图元的类型
	OGRwkbGeometryType GeoType = pLayer->GetGeomType();
	
	//获取图层中的对象个数
	INT64 iFeatureNum = pLayer->GetFeatureCount();

	//---获取属性表结构
	OGRFeatureDefn * pAttribute = pLayer->GetLayerDefn();

	int iFieldNum = pAttribute->GetFieldCount();
	
	vector<FIELD_INFO> FieldInfo;	FIELD_INFO FieldInfoTmp;
	FieldInfo.clear();

	for(int i=0; i<iFieldNum; i++)
	{
		OGRFieldDefn * pField = pAttribute->GetFieldDefn(i);
	
		FieldInfoTmp.strName = pField->GetNameRef();
		FieldInfoTmp.Type = pField->GetType();

		FieldInfo.push_back(FieldInfoTmp);	
	}

	//--读记录
	OGRFeature * pFeature = pLayer->GetFeature(0);

	//获取属性
	vector<CString> FieldValue;

	for(int i=0; i<iFieldNum; i++)
	{
		FieldValue.push_back(pFeature->GetFieldAsString(i));
	}
	//MessageBox(FieldValue);

	//获取空间信息
	OGRGeometry * pGeometry = pFeature->GetGeometryRef();
	OGRwkbGeometryType type = wkbFlatten(pGeometry->getGeometryType());
	OGRPolygon * pOGRPolygon = (OGRPolygon *)pGeometry;
	
	MY_POLYGON PolygonTmp;	MY_POINT PtTmp;	MY_HOLE HoleTmp;

	OGRLinearRing * pRing = pOGRPolygon->getExteriorRing();

	PolygonTmp.iExRingPtNum = pRing->getNumPoints();	

	for(int i=0; i<PolygonTmp.iExRingPtNum; i++)
	{
		PtTmp.dX = pRing->getX(i);
		PtTmp.dY = pRing->getY(i);
		PolygonTmp.ExRingPts.push_back(PtTmp);
	}
	
	PolygonTmp.iInRingNum = pOGRPolygon->getNumInteriorRings();
	for(int i=0; i<PolygonTmp.iInRingNum; i++)
	{
		OGRLinearRing * pHole = pOGRPolygon->getInteriorRing(i);
	
		HoleTmp.iPtNum = pHole ->getNumPoints();
		HoleTmp.Pts.clear();

		for(int j=0; j<HoleTmp.iPtNum; j++)
		{
			PtTmp.dX = pHole->getX(j);
			PtTmp.dY = pHole->getY(j);

			HoleTmp.Pts.push_back(PtTmp);
		}

		PolygonTmp.Holes.push_back(HoleTmp);
	}


	//---创建新文件
	GDALDriver * pDriver = NULL;
	pDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");

	GDALDataset * pDatasetNew = pDriver->Create("D:\\zj_new.shp",0,0,0,GDT_Unknown,NULL);

	OGRLayer * pLayerNew = pDatasetNew->CreateLayer("MyPolygons", SpatialReference, wkbPolygon, NULL);

	//创建属性表
	for(int i=0; i<iFieldNum; i++)
	{
		OGRFieldDefn FieldTmp(FieldInfo[i].strName, FieldInfo[i].Type);
		pLayerNew->CreateField(&FieldTmp);	
	}

	//写空间对象
	OGRFeature * pFeature1 = NULL;
	pFeature1 = OGRFeature::CreateFeature(pLayerNew->GetLayerDefn());

	if(pFeature1 == NULL)
	{
		MessageBox("pFeature = NULL @ WriteGeoObject");
		return;
	}

	//属性
	for(int i=0; i<iFieldNum; i++)
	{
		pFeature1->SetField(FieldInfo[i].strName, FieldValue[i]);
	}

	//空间信息
	OGRPolygon PolygonTmp1;
	PolygonTmp1.empty();

	//先外环后内环
	OGRLinearRing RingTmp; 
	for(int i=0; i<PolygonTmp.iExRingPtNum; i++)
	{
		RingTmp.addPoint(PolygonTmp.ExRingPts[i].dX, PolygonTmp.ExRingPts[i].dY);
	}
	
	RingTmp.closeRings();
	PolygonTmp1.addRing(&RingTmp);

	RingTmp.empty();
	for(int i=0; i<PolygonTmp.iInRingNum; i++)
	{
		RingTmp.empty();
		for(int j=0; j<PolygonTmp.Holes[i].iPtNum; j++)
		{
			RingTmp.addPoint(PolygonTmp.Holes[i].Pts[j].dX, PolygonTmp.Holes[i].Pts[j].dX);
		}		
		RingTmp.closeRings();
		PolygonTmp1.addRing(&RingTmp);
	}

	OGRErr Ret = pFeature1->SetGeometry(&PolygonTmp1);

	Ret = pLayerNew->CreateFeature(pFeature1);


	int x =0;


}

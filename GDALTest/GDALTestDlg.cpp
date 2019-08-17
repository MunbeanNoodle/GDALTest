// GDALTestDlg.cpp : ʵ���ļ�
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
	//�⻷
	int iExRingPtNum;
	vector<MY_POINT> ExRingPts;

	//�ڻ�
	int iInRingNum;
	vector<MY_HOLE> Holes;		
}MY_POLYGON;


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CGDALTestDlg �Ի���




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


// CGDALTestDlg ��Ϣ�������

BOOL CGDALTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	GDALAllRegister();

	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");
	CPLSetConfigOption("SHAPE_ENCODING","");


	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CGDALTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ��������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù����ʾ��
//
HCURSOR CGDALTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CGDALTestDlg::OnBnClickedBnReadImage()
{
	//����һ�����ݼ���ͼ������
	GDALDataset * poDataset;	

	//��ͼ���ļ�
	poDataset = (GDALDataset *)GDALOpen("D:\\Fusion.tif", GA_ReadOnly);	

	//�����ʧ�ܣ��򱨴�
	if(poDataset == NULL){MessageBox("Open file error!");	return;}

	//---��ȡͼ�������Ϣ
	
	//��ȡͼ���ʽ
	CString str = poDataset->GetDriver()->GetDescription();
	
	//��ȡXY��С
	int iRows = poDataset->GetRasterYSize();
	int iCols = poDataset->GetRasterXSize();
	
	//��ȡ������
	int iBands = poDataset->GetRasterCount();
	
	//��ȡͶӰ�ַ���
	CString str1 = GDALGetProjectionRef(poDataset);
	
	//��ȡ����任ϵ��
	double dTrans[6];	
	poDataset->GetGeoTransform(dTrans);

	//����������
	BYTE * pData = NULL;
	pData = (BYTE *)malloc(iRows * iCols * 1 * sizeof(BYTE));
	if(pData == NULL){MessageBox("malloc error!"); return;}

	GDALRasterBand * pBand = poDataset->GetRasterBand(3);
	CPLErr Ret = pBand->RasterIO(GF_Read, 0,0,iCols,iRows,pData,iCols,iRows,GDT_Byte,0,0);

	//��ȡԪ����
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

	//����ͼ��
	GDALDriver * pDriver = NULL;
	pDriver = GetGDALDriverManager()->GetDriverByName("GTIFF");

	GDALDataset * poDatasetW = pDriver->Create("D:\\new.tif",  iCols,  iRows,  1, GDT_Byte,  NULL);
	if(poDatasetW == NULL){MessageBox("Create file error��"); return;}

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
	//��ʸ���ļ�
	GDALDataset * pDataset;
	
	pDataset = (GDALDataset *)GDALOpenEx("D:\\zj_Polygon.shp", GDAL_OF_VECTOR, NULL, NULL, NULL);

	//��ȡͼ������
	int LayerNum = pDataset->GetLayerCount();

	//��ȡͼ����,�±��0��ʼ
	OGRLayer * pLayer = pDataset->GetLayer(0);

	//��ȡͼ��ռ䷶Χ
	OGREnvelope Envelope;
	pLayer->GetExtent(&Envelope);

	//��ȡͼ������
	CString str =pLayer->GetName();
	
	//��ȡͼ��ռ�ο�
	OGRSpatialReference * SpatialReference = pLayer->GetSpatialRef();

	//��ȡͼ����ͼԪ������
	OGRwkbGeometryType GeoType = pLayer->GetGeomType();
	
	//��ȡͼ���еĶ������
	INT64 iFeatureNum = pLayer->GetFeatureCount();

	//---��ȡ���Ա�ṹ
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

	//--����¼
	OGRFeature * pFeature = pLayer->GetFeature(0);

	//��ȡ����
	vector<CString> FieldValue;

	for(int i=0; i<iFieldNum; i++)
	{
		FieldValue.push_back(pFeature->GetFieldAsString(i));
	}
	//MessageBox(FieldValue);

	//��ȡ�ռ���Ϣ
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


	//---�������ļ�
	GDALDriver * pDriver = NULL;
	pDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");

	GDALDataset * pDatasetNew = pDriver->Create("D:\\zj_new.shp",0,0,0,GDT_Unknown,NULL);

	OGRLayer * pLayerNew = pDatasetNew->CreateLayer("MyPolygons", SpatialReference, wkbPolygon, NULL);

	//�������Ա�
	for(int i=0; i<iFieldNum; i++)
	{
		OGRFieldDefn FieldTmp(FieldInfo[i].strName, FieldInfo[i].Type);
		pLayerNew->CreateField(&FieldTmp);	
	}

	//д�ռ����
	OGRFeature * pFeature1 = NULL;
	pFeature1 = OGRFeature::CreateFeature(pLayerNew->GetLayerDefn());

	if(pFeature1 == NULL)
	{
		MessageBox("pFeature = NULL @ WriteGeoObject");
		return;
	}

	//����
	for(int i=0; i<iFieldNum; i++)
	{
		pFeature1->SetField(FieldInfo[i].strName, FieldValue[i]);
	}

	//�ռ���Ϣ
	OGRPolygon PolygonTmp1;
	PolygonTmp1.empty();

	//���⻷���ڻ�
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

#include "StdAfx.h"
#include "VectorIO.h"

CString CVectorIO::ConvertTextInCoding(const char * pText, int iCodingCur)
{
	CString strRet;

	//��ת��
	if(iCodingCur == -1)
	{
		strRet = pText;
	}

	//UTF-8ת��
	else if(iCodingCur == 0)
	{
		int unicodeLen = ::MultiByteToWideChar( CP_UTF8, 0,pText, -1,NULL,0 ); 		
		MultiByteToWideChar( CP_UTF8, 0, pText, -1, (LPWSTR)pUnicode, unicodeLen );
		strRet = pUnicode;
	}

	return strRet;
}

BOOL CVectorIO::GetTextCodingFromFileForShape(CString strName)
{
	//�����Ƿ���CPG�ļ�
	CFileFind finder;
	BOOL bRet = finder.FindFile(strName.Left(strName.GetLength() - 3) + "cpg");

	if(!bRet)
		return FALSE;

	//����������CPG�ļ�
	CStdioFile File;
	CString CPGFile = strName.Left(strName.GetLength() - 3) + "cpg";
	if(!File.Open(CPGFile,CFile::modeRead | CFile::typeText,NULL))
	{
		AfxMessageBox("Open CPG File Error! @ CVectorIO::GetTextCodingFromFileForShape");
		return FALSE;
	}

	//��ȡ���뷽ʽ�ַ���
	CString strLine;
	File.ReadString(strLine);

	if(strLine == "")
	{
		//���ù�
	}
	if(strLine == "UTF-8")
	{
		iCoding = 0;	//ָ�����뷽ʽΪǿ��UTF-8ת��
	}
	else
	{
		AfxMessageBox("Cannot tell CPF File!");	//����һ���µı��뷽ʽ,�Ա��һ���������ƴ���
	}

	return TRUE;
}

CVectorIO::CVectorIO(void)
{
	//ע������
	GDALAllRegister();

	//OGRRegisterAll(); 

	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");
	CPLSetConfigOption("SHAPE_ENCODING","");

	pDataset = NULL;

	bErrLogOpen = FALSE;

	bDeleteFeature = FALSE;

	InitDataVessel();

	//---��ʼ���ļ�����ѡ���б�
	Map_SHAPE_OPTION_LIST.insert(pair<CString, CString>("ADJUST_TYPE",""));				//YES/NO: Set to YES (default is NO) to read the whole .dbf to adjust Real->Integer/Integer64 or Integer64->Integer field types when possible. This can be used when field widths are ambiguous and that by default OGR would select the larger data type. For example, a numeric column with 0 decimal figures and with width of 10/11 character may hold Integer or Integer64, and with width 19/20 may hold Integer64 or larger integer (hold as Real)
	Map_SHAPE_OPTION_LIST.insert(pair<CString, CString>("ADJUST_GEOM_TYPE",""));		//NO/FIRST_SHAPE/ALL_SHAPES. (Starting with GDAL 2.1) Defines how layer geometry type is computed, in particular to distinguish shapefiles that have shapes with significant values in the M dimension from the ones where the M values are set to the nodata value. By default (FIRST_SHAPE), the driver will look at the first shape and if it has M values it will expose the layer as having a M dimension. By specifying ALL_SHAPES, the driver will iterate over features until a shape with a valid M value is found to decide the appropriate layer type.
	Map_SHAPE_OPTION_LIST.insert(pair<CString, CString>("SHPT",""));					//Override the type of shapefile created. Can be one of NULL for a simple .dbf file with no .shp file, POINT, ARC, POLYGON or MULTIPOINT for 2D; POINTZ, ARCZ, POLYGONZ or MULTIPOINTZ for 3D; POINTM, ARCM, POLYGONM or MULTIPOINTM for measured geometries; and POINTZM, ARCZM, POLYGONZM or MULTIPOINTZM for 3D measured geometries. The measure support was added in GDAL 2.1. MULTIPATCH files are not supported.
	Map_SHAPE_OPTION_LIST.insert(pair<CString, CString>("ENCODING",""));				//value: set the encoding value in the DBF file. The default value is "LDID/87". It is not clear what other values may be appropriate.
	Map_SHAPE_OPTION_LIST.insert(pair<CString, CString>("RESIZE",""));					//YES/NO: (OGR >= 1.10.0) set the YES to resize fields to their optimal size. See above "Field sizes" section. Defaults to NO.
	Map_SHAPE_OPTION_LIST.insert(pair<CString, CString>("2GB_LIMIT",""));				//YES/NO: (OGR >= 1.11) set the YES to enforce the 2GB file size for .SHP or .DBF files. Defaults to NO.
	Map_SHAPE_OPTION_LIST.insert(pair<CString, CString>("SPATIAL_INDEX",""));			//YES/NO: (OGR >= 2.0) set the YES to create a spatial index (.qix). Defaults to NO.
	Map_SHAPE_OPTION_LIST.insert(pair<CString, CString>("DBF_DATE_LAST_UPDATE",""));	//YYYY-MM-DD: (OGR >= 2.0) Modification date to write in DBF header with year-month-day format. If not specified, current date is used. Note: behaviour of past GDAL releases was to write 1995-07-26

	Map_TABMIF_OPTION_LIST.insert(pair<CString, CString>("FORMAT",""));					//MIF: To create MIF/MID instead of TAB files (TAB is the default).
	Map_TABMIF_OPTION_LIST.insert(pair<CString, CString>("SPATIAL_INDEX_MODE",""));		//QUICK/OPTIMIZED: The default is QUICK force "quick spatial index mode". In this mode writing files can be about 5 times faster, but spatial queries can be up to 30 times slower. This can be set to OPTIMIZED in GDAL 2.0 to generate optimized spatial index.
	Map_TABMIF_OPTION_LIST.insert(pair<CString, CString>("BLOCKSIZE",""));				//[512,1024,...,32256] (multiples of 512): (GDAL >= 2.0.2) Block size for .map files. Defaults to 512. GDAL 2.0.1 and earlier versions only supported BLOCKSIZE=512 in reading and writing. MapInfo 15.2 and above creates .tab files with a blocksize of 16384 bytes. Any MapInfo version should be able to handle block sizes from 512 to 32256.
	Map_TABMIF_OPTION_LIST.insert(pair<CString, CString>("BOUNDS",""));					//xmin,ymin,xmax,ymax: (GDAL >=2.0) Define custom layer bounds to increase the accuracy of the coordinates. Note: the geometry of written features must be within the defined box.

	//Ĭ�ϲ���������ǿ��ת��
	iCoding = -1;

	//����һ�������ַ�ת���ռ�
	pUnicode= new  wchar_t[MAX_TEXT_CONVERT_MEM];
}

CVectorIO::~CVectorIO(void)
{
	iCoding = -1;

	delete []pUnicode;

	CloseFile();
}

BOOL CVectorIO::InitDataVessel()
{
	//�رվ��
	if(pDataset != NULL)
	{
		GDALClose(pDataset);		pDataset = NULL;
	}

	strFilePath = "";

	strFileExt = "";	

	BasicInfo.enFileType = UNKNOWN_FILE;
	BasicInfo.iFileStatus = -1;
	BasicInfo.iLayerNum = 0;
	memset(BasicInfo.dBounds,0,sizeof(double) * 4);
	BasicInfo.GeometryType.clear();
	BasicInfo.i64FeatureNum.clear();
	BasicInfo.strLayerName.clear();
	BasicInfo.SpatialRef.clear();
	BasicInfo.Attribute.clear();

	if(bErrLogOpen)
	{
		ErrLog.Close();

		bErrLogOpen = FALSE;
	}

	return TRUE;
}

BOOL CVectorIO::CloseFile()
{
	//REPACK TABLE
	if(bDeleteFeature)
	{
		if(BasicInfo.enFileType == ESRI_SHAPE_FILE)
		{
			OGRLayer * pLayer; CString strSQL;
			for(int i=0; i<BasicInfo.iLayerNum; i++)
			{
				strSQL.Format("REPACK %s",BasicInfo.strLayerName[i]);
				pLayer = pDataset->ExecuteSQL(strSQL,NULL,"");

				if(pLayer == NULL)
				{
					strSQL.Format("pDataset->ExecuteSQL Error! for Layer[%d] @ CVectorIO::CloseFile()",i);
					strErrMsgs.push_back(strSQL);
					return FALSE;
				}
			}
		}

		if(BasicInfo.enFileType == MAPINFO_TAB_FILE)
		{
			//TAB�ļ���Ҫ�½�
			OGREnvelope Envelope;
			pDataset->GetLayer(0)->GetExtent(&Envelope);	
			BasicInfo.dBounds[0] = Envelope.MinX;	BasicInfo.dBounds[1] = Envelope.MinY;	
			BasicInfo.dBounds[2] = Envelope.MaxX;	BasicInfo.dBounds[3] = Envelope.MaxY;

			CString strPathTmp = strFilePath.Left(strFilePath.GetLength() - 4);

			CVectorIO TmpFile;

			BOOL bRet = TmpFile.CreateFile(strPathTmp + "_tmp.tab",BasicInfo);

			OGRFeature * pFeatureTmp = NULL;	INT64 i64nFID = 0;
			GEO_OBJECT_SPATIAL ObjSpatial;		GEO_OBJECT_ATTRIBUTE ObjAttribute;

			pDataset->GetLayer(0)->ResetReading();
			while((pFeatureTmp = pDataset->GetLayer(0)->GetNextFeature()) != NULL)
			{
				i64nFID = pFeatureTmp->GetFID() - 1;

				ReadGeoObject(i64nFID,ObjSpatial,ObjAttribute,TRUE,TRUE);

				TmpFile.WriteGeoObject(0,ObjSpatial,ObjAttribute,TRUE,TRUE,TRUE);
			}

			TmpFile.CloseFile();

			if(pDataset != NULL)
			{
				GDALClose(pDataset);		pDataset = NULL;
			}

			//�滻�ļ�
			bRet = FALSE;	int iRet = -1;

			bRet = DeleteFile(strPathTmp + ".tab");
			if(!bRet)
			{
				strErrMsgs.push_back("delete *.tab error @ CVectorIO::CloseFile()");
				return FALSE;
			}

			bRet = DeleteFile(strPathTmp + ".dat");
			if(!bRet)
			{
				strErrMsgs.push_back("delete *.dat error @ CVectorIO::CloseFile()");
				return FALSE;
			}

			bRet = DeleteFile(strPathTmp + ".map");
			if(!bRet)
			{
				strErrMsgs.push_back("delete *.map error @ CVectorIO::CloseFile()");
				return FALSE;
			}

			bRet = DeleteFile(strPathTmp + ".id");
			if(!bRet)
			{
				strErrMsgs.push_back("delete *.id error @ CVectorIO::CloseFile()");
				return FALSE;
			}

			iRet = rename(strPathTmp + "_tmp.tab",strPathTmp + ".tab");
			if(iRet != 0)
			{
				strErrMsgs.push_back("rename *.tab error @ CVectorIO::CloseFile()");
				return FALSE;
			}

			iRet = rename(strPathTmp + "_tmp.dat",strPathTmp + ".dat");
			if(iRet != 0)
			{
				strErrMsgs.push_back("rename *.dat error @ CVectorIO::CloseFile()");
				return FALSE;
			}

			iRet = rename(strPathTmp + "_tmp.map",strPathTmp + ".map");
			if(iRet != 0)
			{
				strErrMsgs.push_back("rename *.map error @ CVectorIO::CloseFile()");
				return FALSE;
			}

			iRet = rename(strPathTmp + "_tmp.id",strPathTmp + ".id");
			if(iRet != 0)
			{
				strErrMsgs.push_back("rename *.id error @ CVectorIO::CloseFile()");
				return FALSE;
			}

			bDeleteFeature = FALSE;
		}
	}
	else
	{
		InitDataVessel();
	}	

	return TRUE;
}

BOOL CVectorIO::GetErrorMessage(BOOL bMsgBox, BOOL bExportErrLog)
{
	CString str = "";

	if(strErrMsgs.size() > 0)
	{
		for(string::size_type i=0; i<strErrMsgs.size(); i++)
		{
			str = str + strErrMsgs[i] + "\n";
		}
	}
	else
	{
		str = "This is no error message!";
	}

	if(bMsgBox)
		AfxMessageBox(str);

	if(bExportErrLog)
	{
		if(!bErrLogOpen)
		{
			CString strErrLogPath;

			if(strFilePath = "")
			{
				strErrLogPath = ".\\ErrLog.txt";
			}
			else
			{
				strErrLogPath= strFilePath.Left(strFilePath.GetLength() - 4) + "_ErrLog.txt";
			}

			if(!ErrLog.Open(strErrLogPath, CFile::modeCreate | CFile::modeWrite | CFile::typeText, NULL))
			{
				AfxMessageBox("Create ErrLog File Error! Can not Export the Error Message to the Log File! @ CVectorIO::GetErrorMessage");
				bErrLogOpen = FALSE;
				return FALSE;
			}
			else
			{
				bErrLogOpen = TRUE;
			}
		}

		ErrLog.WriteString(str);
	}

	return TRUE;
}

INT64 CVectorIO::GetFeatureNum(OGRLayer * pLayer)
{
	INT64 iFeatureNum = 0;

	pLayer->ResetReading();

	while(pLayer->GetNextFeature())
	{
		iFeatureNum++;
	}

	pLayer->ResetReading();

	return iFeatureNum;
}

BOOL CVectorIO::OpenFile(CString strFileName, VECTOR_BASIC_INFO & VectorInfo, int iForceCoding /* = 0 */)
{
	strErrMsgs.clear();

	//�����������
	if(BasicInfo.iFileStatus != -1)
	{
		InitDataVessel();
	}

	//��ȡ�ļ�·������չ��������ļ�����
	strFilePath = strFileName;	strFileExt = strFileName.Right(3);	strFileExt.MakeUpper();

	if(strFileExt == "SHP")
	{
		BasicInfo.enFileType = ESRI_SHAPE_FILE;
	}
	else if(strFileExt == "TAB"){BasicInfo.enFileType = MAPINFO_TAB_FILE;}
	else if(strFileExt == "MIF"){BasicInfo.enFileType = MAPINFO_MIF_FILE;}
	else{strErrMsgs.push_back("Not Support Now! @ CVectorIO::OpenFile");	return FALSE;}

	//MIF�ļ���֧����UPDATE��ʽ��
	if(BasicInfo.enFileType == MAPINFO_MIF_FILE)
	{
		pDataset = (GDALDataset *)GDALOpenEx(strFileName,GDAL_OF_VECTOR,NULL,NULL,NULL);
	}
	else
	{
		pDataset = (GDALDataset *)GDALOpenEx(strFileName,GDAL_OF_VECTOR | GDAL_OF_UPDATE,NULL,NULL,NULL);

		//���±���ת����ʶ
		iCoding = iForceCoding;

		//�����Ƿ���CPG�ļ���ʶ������
		GetTextCodingFromFileForShape(strFileName);
	}

	if(pDataset == NULL){strErrMsgs.push_back("pDataset == NULL; @ CVectorIO::OpenFile");	return FALSE;}

	//��ȡͼ����
	BasicInfo.iLayerNum = pDataset->GetLayerCount();

	OGRLayer * pLayer = NULL;
	OGRFeatureDefn * pAttribute = NULL;
	OGRFieldDefn * pField = NULL;

	ATTRIBUTE_INFO AttributeInfo;

	//��ȡ��ͼ���¼��
	for(int i=0; i<BasicInfo.iLayerNum; i++)
	{
		//��ȡ��ǰ����ͼ����
		pLayer = pDataset->GetLayer(i);
		if(pLayer == NULL){strErrMsgs.push_back("pLayer == NULL @ CVectorIO::OpenFile");	return FALSE;}

		//��ȡ�ռ䷶Χ��ֻ���ǵ�һ�㣩
		if(i==0)
		{
			OGREnvelope Envelope;
			pDataset->GetLayer(0)->GetExtent(&Envelope);
			BasicInfo.dBounds[0] = Envelope.MinX;	BasicInfo.dBounds[1] = Envelope.MinY;	
			BasicInfo.dBounds[2] = Envelope.MaxX;	BasicInfo.dBounds[3] = Envelope.MaxY;
		}

		//��ȡ��ͼ������
		BasicInfo.strLayerName.push_back(pLayer->GetName());

		//��ȡ�ռ�ο�
		OGRSpatialReference * SpatialReference = NULL;
		SpatialReference = pLayer->GetSpatialRef();
		if(SpatialReference != NULL)
			BasicInfo.SpatialRef.push_back(*SpatialReference);

		//��ȡ��ͼ��ͼԪ����
		BasicInfo.GeometryType.push_back(pLayer->GetGeomType());

		//��ȡ��ͼ��ռ������Ŀ
		//���Ϊ0�Ļ�,��Ҫ���Ǹ�ͼ��û�б�REPACK�����,�����ͨ�������ķ���ͳ��һ�¶�����
		INT64 iFeatureNum = pLayer->GetFeatureCount();
		//if(iFeatureNum <= 0)
		iFeatureNum = GetFeatureNum(pLayer);

		BasicInfo.i64FeatureNum.push_back(iFeatureNum);

		//��ȡ��ͼ�����Ա�
		pAttribute = pLayer->GetLayerDefn();
		if(pAttribute == NULL){strErrMsgs.push_back("pAttribute == NULL @ CVectorIO::OpenFile");	return FALSE;}

		//�������Ա�������������Ϣ�ṹ��
		AttributeInfo.iFieldNum = pAttribute->GetFieldCount();
		for(int i=0; i<AttributeInfo.iFieldNum; i++)
		{
			pField = pAttribute->GetFieldDefn(i);
			if(pField == NULL){strErrMsgs.push_back("pField == NULL @ CVectorIO::OpenFile");	return FALSE;}

			AttributeInfo.enFieldType.push_back(pField->GetType());
			AttributeInfo.strFieldName.push_back(ConvertTextInCoding(pField->GetNameRef(),iCoding));

		}
		BasicInfo.Attribute.push_back(AttributeInfo);		
	}

	//����ļ���״̬
	BasicInfo.iFileStatus = 0;

	//����������Ϣ
	VectorInfo.enFileType = BasicInfo.enFileType;
	VectorInfo.iFileStatus = BasicInfo.iFileStatus;
	VectorInfo.iLayerNum = BasicInfo.iLayerNum;
	VectorInfo.dBounds[0] = BasicInfo.dBounds[0];
	VectorInfo.dBounds[1] = BasicInfo.dBounds[1];
	VectorInfo.dBounds[2] = BasicInfo.dBounds[2];
	VectorInfo.dBounds[3] = BasicInfo.dBounds[3];
	VectorInfo.i64FeatureNum.clear();
	VectorInfo.strLayerName.clear();
	VectorInfo.Attribute.clear();
	for(int i=0; i<BasicInfo.iLayerNum; i++)
	{
		VectorInfo.GeometryType.push_back(BasicInfo.GeometryType[i]);
		VectorInfo.i64FeatureNum.push_back(BasicInfo.i64FeatureNum[i]);
		VectorInfo.strLayerName.push_back(BasicInfo.strLayerName[i]);
		VectorInfo.Attribute.push_back(BasicInfo.Attribute[i]);
		if((int)BasicInfo.SpatialRef.size() > i)
			VectorInfo.SpatialRef.push_back(BasicInfo.SpatialRef[i]);
	}

	return TRUE;
}

BOOL CVectorIO::CreateFile(CString strFileName, VECTOR_BASIC_INFO VectorInfo)
{
	strErrMsgs.clear();

	//�����������
	if(BasicInfo.iFileStatus != -1)
	{
		InitDataVessel();
	}

	//��������
	strFilePath = strFileName;	strFileExt = strFileName.Right(3);	strFileExt.MakeUpper();
	GDALDriver * pDriver = NULL;

	if(strFileExt == "SHP")
	{
		pDriver = GetGDALDriverManager()->GetDriverByName("ESRI Shapefile");	BasicInfo.enFileType = ESRI_SHAPE_FILE;
	}
	else if(strFileExt == "TAB" || strFileExt == "MIF")
	{
		pDriver = GetGDALDriverManager()->GetDriverByName("MapInfo File");	

		if(strFileExt == "TAB"){BasicInfo.enFileType = MAPINFO_TAB_FILE;}else{BasicInfo.enFileType = MAPINFO_MIF_FILE;}
	}
	else
	{
		strErrMsgs.push_back("Not Support Now! @ CVectorIO::CreateFile");		GDALDestroyDriverManager();		return FALSE;
	}
	if(pDriver == NULL){strErrMsgs.push_back("pField == NULL @ CreateFile"); return FALSE;}

	//��ȡ����ѡ��
	char ** papszOptions = NULL;	

	map<CString, CString>::iterator iter;

	if(strFileExt == "SHP")
	{
		for(iter=Map_SHAPE_OPTION_LIST.begin(); iter!=Map_SHAPE_OPTION_LIST.end(); iter++)
		{
			if(iter->second != "")
			{
				papszOptions = CSLSetNameValue(papszOptions,iter->first,iter->second);
			}
		}
	}
	else if(strFileExt == "TAB" || strFileExt == "MIF")
	{
		for(iter=Map_TABMIF_OPTION_LIST.begin(); iter!=Map_TABMIF_OPTION_LIST.end(); iter++)
		{
			if(iter->second != "")
			{
				papszOptions = CSLSetNameValue(papszOptions,iter->first,iter->second);
			}
		}

		if(strFileExt == "MIF")
		{
			papszOptions = CSLSetNameValue(papszOptions,"FORMAT","MIF");
		}

		CString strBounds;
		strBounds.Format("%f,%f,%f,%f",VectorInfo.dBounds[0],VectorInfo.dBounds[1],VectorInfo.dBounds[2],VectorInfo.dBounds[3]);
		papszOptions = CSLSetNameValue(papszOptions,"BOUNDS",strBounds);
	}	

	//�����ļ�
	pDataset = pDriver->Create(strFileName,0,0,0,GDT_Unknown,papszOptions);
	if(pDataset == NULL){strErrMsgs.push_back("pField == NULL @ CreateFile");	return FALSE;}

	//����ͼ��
	for(int i=0; i<VectorInfo.iLayerNum; i++)
	{
		//ͼ����
		CString strLayerName = "";
		if(string::size_type(i) >= VectorInfo.strLayerName.size()){strLayerName.Format("Layer %d",i);}
		else
		{
			if(VectorInfo.strLayerName[i] == ""){strLayerName.Format("Layer %d",i);}
			else{strLayerName = VectorInfo.strLayerName[i];}
		}

		//�ռ�ο�
		OGRSpatialReference SpatialRef;		SpatialRef.Clear();
		if(string::size_type(i) < VectorInfo.SpatialRef.size())
		{
			SpatialRef = VectorInfo.SpatialRef[i];
		}
		else
		{
			if(strFileExt == "TAB" || strFileExt == "MIF")
			{
				//ĿǰĬ��NonEarth����ϵͳ
				CString str = "CoordSys NonEarth Units \"m\"";
				SpatialRef.importFromMICoordSys(str);
			}
		}

		//����ͼ��
		OGRLayer * pLayer = NULL;
		pLayer = pDataset->CreateLayer(strLayerName,&SpatialRef,VectorInfo.GeometryType[i],papszOptions);
		if(pLayer == NULL){strErrMsgs.push_back("pField == NULL @ CreateFile");	return FALSE;}

		//�������Ա�
		if(string::size_type(i) >= VectorInfo.Attribute.size()){return FALSE;}
		for(int k=0; k<VectorInfo.Attribute[i].iFieldNum; k++)
		{
			if(VectorInfo.Attribute[i].enFieldType[k] == OFTInteger64){VectorInfo.Attribute[i].enFieldType[k] = OFTReal;}

			OGRFieldDefn FieldTmp(VectorInfo.Attribute[i].strFieldName[k],VectorInfo.Attribute[i].enFieldType[k]);			

			if(VectorInfo.Attribute[i].enFieldType[k] == OFTString || VectorInfo.Attribute[i].enFieldType[k] == OFTStringList)
			{
				FieldTmp.SetWidth(256);
			}

			OGRErr Err = pLayer->CreateField(&FieldTmp);
			if(Err != OGRERR_NONE)
			{
				CString str;	str.Format("CreateField Err: %d @ CreateFile",Err);
				strErrMsgs.push_back(str);	return FALSE;
			}
		}
	}


	//���»�����Ϣ
	//�Ѹ���BasicInfo.iFileType = -1;
	BasicInfo.iFileStatus = 1;
	BasicInfo.iLayerNum = VectorInfo.iLayerNum;
	BasicInfo.dBounds[0] = VectorInfo.dBounds[0];
	BasicInfo.dBounds[1] = VectorInfo.dBounds[1];
	BasicInfo.dBounds[2] = VectorInfo.dBounds[2];
	BasicInfo.dBounds[3] = VectorInfo.dBounds[3];
	BasicInfo.i64FeatureNum.clear();
	BasicInfo.strLayerName.clear();
	BasicInfo.SpatialRef.clear();
	BasicInfo.Attribute.clear();
	for(int i=0; i<BasicInfo.iLayerNum; i++)
	{
		BasicInfo.GeometryType.push_back(VectorInfo.GeometryType[i]);
		//BasicInfo.i64FeatureNum.push_back(Info.i64FeatureNum[i]);
		BasicInfo.strLayerName.push_back(VectorInfo.strLayerName[i]);
		if((int)VectorInfo.SpatialRef.size() > i)
			BasicInfo.SpatialRef.push_back(VectorInfo.SpatialRef[i]);
		BasicInfo.Attribute.push_back(VectorInfo.Attribute[i]);
	}

	return TRUE;
}


BOOL CVectorIO::ReadGeoObject(INT64 i64FeatureNo, GEO_OBJECT_SPATIAL & GeoObjSpatial, GEO_OBJECT_ATTRIBUTE & GeoObjAttribute, BOOL bReadSpatialInfo, BOOL bReadAttributeInfo, int iLayerNo /* = 0 */)
{
	strErrMsgs.clear();

	//��ȫ��飺����ļ�δ��, ��ֱ���˳�
	if(BasicInfo.iFileStatus < 0)
	{
		CString str;	str.Format("iFileStatus = %d @ ReadSpatialInfo",BasicInfo.iFileStatus);
		strErrMsgs.push_back(str);
		return FALSE;
	}

	//��ȫ��飺���ָ��iLayerNo������ͼ�����򲻺Ϸ�, ��ֱ���˳�
	if(iLayerNo >= BasicInfo.iLayerNum || iLayerNo < 0)
	{
		CString str;	str.Format("iLayerNo = %d @ ReadSpatialInfo",iLayerNo);
		strErrMsgs.push_back(str);
		return FALSE;
	}

	//��ȫ��飺���ָ��i64FeatureNo�����ܶ������򲻺Ϸ�, ��ֱ���˳�
	//Mapinfo�Ŀռ�����±��1��ʼ,Ϊ��ͳһ����,�ڴ����⴦��һ��
	if(BasicInfo.enFileType == MAPINFO_MIF_FILE || BasicInfo.enFileType == MAPINFO_TAB_FILE)
	{
		i64FeatureNo++;
	}
	if(i64FeatureNo > BasicInfo.i64FeatureNum[iLayerNo] || i64FeatureNo < 0)
	{
		CString str;	str.Format("i64FeatureNo = %d @ ReadSpatialInfo",i64FeatureNo);
		strErrMsgs.push_back(str);
		return FALSE;
	}

	//��ȡ����ͼ����
	OGRLayer * pLayer = NULL;
	pLayer = pDataset->GetLayer(iLayerNo);
	if(pLayer == NULL)
	{
		strErrMsgs.push_back("pLayer = NULL @ ReadSpatialInfo");
		return FALSE;
	}

	//pLayer->ResetReading();


	//��ȡ�����ռ������
	OGRFeature * pFeature = NULL;

	/*pFeature = pLayer->GetNextFeature();
	INT64 iTmp = pFeature->GetFID();*/


	pFeature = pLayer->GetFeature(i64FeatureNo);
	if(pFeature == NULL)
	{
		strErrMsgs.push_back("pFeature = NULL @ ReadSpatialInfo");
		return FALSE;
	}


	//��ȡ������Ϣ
	if(bReadAttributeInfo)
	{
		//�����������
		GeoObjAttribute.uiFieldNum = 0;
		GeoObjAttribute.FieldInfoString.clear();

		//���������������Ա���������
		GeoObjAttribute.uiFieldNum = BasicInfo.Attribute[iLayerNo].iFieldNum;	

		for(int i=0; i<BasicInfo.Attribute[iLayerNo].iFieldNum; i++)
		{
			GeoObjAttribute.FieldInfoString.push_back(ConvertTextInCoding(pFeature->GetFieldAsString(i),iCoding));
		}
	}

	//��ȡ�ռ���Ϣ
	if(bReadSpatialInfo)
	{
		//�����������
		GeoObjSpatial.MainGeometry.GeometryType = UNKNOWN_TYPE;
		GeoObjSpatial.MainGeometry.uiNodePointNum = 0;
		GeoObjSpatial.MainGeometry.NodePointList.clear();
		GeoObjSpatial.MainGeometry.uiRingNum = 0;
		GeoObjSpatial.MainGeometry.Rings.clear();
		GeoObjSpatial.MainGeometry.uiSubGeometryNum = 0;
		GeoObjSpatial.SubGeometrys.clear();
		GeoObjSpatial.PartGeometrys.clear();

		//��ȡ�ռ����Ŀռ���Ϣ���
		OGRGeometry * pGeometry = NULL;	
		pGeometry = pFeature->GetGeometryRef();
		if(pGeometry == NULL)
		{
			strErrMsgs.push_back("pGeometry = NULL @ ReadSpatialInfo");	
			OGRFeature::DestroyFeature(pFeature);
			return FALSE;
		}

		//���ռ����Ŀռ���Ϣ��ֵ����׼����	
		OGRwkbGeometryType type = wkbFlatten(pGeometry->getGeometryType());
		GeoObjSpatial.MainGeometry.GeometryType = (GEOMETRY_TYPE)wkbFlatten(pGeometry->getGeometryType());

		//�����ͽ��д���,���������ͳһ��GEO_OBJECT_SPATIAL�ṹ����,�������û���˵ʼ�ղ����ľ���һ�����͵�����,�����˽ӿڵ������Ժ�������
		NODE_POINT PT;	RING Ring;
		OGRLinearRing * pRing = NULL;
		OGRLinearRing * pIslandRing = NULL;

		//������	
		if(GeoObjSpatial.MainGeometry.GeometryType == SINGLE_POINT)
		{
			OGRPoint * pOGRPoint = (OGRPoint *)pGeometry;

			GeoObjSpatial.MainGeometry.uiNodePointNum = 1;
			PT.dX = pOGRPoint->getX();
			PT.dY = pOGRPoint->getY();
			GeoObjSpatial.MainGeometry.NodePointList.push_back(PT);

			OGRFeature::DestroyFeature(pFeature);

			return TRUE;
		}

		//������
		else if(GeoObjSpatial.MainGeometry.GeometryType == SINGLE_POLYLINE)
		{
			OGRLineString * pOGRLine = (OGRLineString *)pGeometry;

			GeoObjSpatial.MainGeometry.uiNodePointNum = pOGRLine->getNumPoints();
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiNodePointNum; i++)
			{
				PT.dX = pOGRLine->getX(i);	PT.dY = pOGRLine->getY(i);
				GeoObjSpatial.MainGeometry.NodePointList.push_back(PT);
			}

			OGRFeature::DestroyFeature(pFeature);

			return TRUE;
		}

		//���������
		else if(GeoObjSpatial.MainGeometry.GeometryType == SINGLE_POLYGON)
		{
			OGRPolygon * pOGRPolygon = (OGRPolygon *)pGeometry;

			//��ȡ������
			pRing = pOGRPolygon->getExteriorRing();
			GeoObjSpatial.MainGeometry.uiNodePointNum = pRing->getNumPoints();
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiNodePointNum; i++)
			{
				PT.dX = pRing->getX(i);	PT.dY = pRing->getY(i);
				GeoObjSpatial.MainGeometry.NodePointList.push_back(PT);
			}

			//��ȡ�ڶ����
			GeoObjSpatial.MainGeometry.uiRingNum = pOGRPolygon->getNumInteriorRings();
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiRingNum; i++)
			{
				pIslandRing = pOGRPolygon->getInteriorRing(i);
				Ring.NodePointList.clear();
				Ring.uiNodePointNum = pIslandRing->getNumPoints();
				for(unsigned int k=0; k<Ring.uiNodePointNum; k++)
				{
					PT.dX = pIslandRing->getX(k);	PT.dY = pIslandRing->getY(k);
					Ring.NodePointList.push_back(PT);
				}
				GeoObjSpatial.MainGeometry.Rings.push_back(Ring);
			}

			OGRFeature::DestroyFeature(pFeature);

			return TRUE;
		}

		//��ϵ�
		else if(GeoObjSpatial.MainGeometry.GeometryType == MULTI_POINTS)
		{
			OGRMultiPoint * pMultiPoint = (OGRMultiPoint *)pGeometry;

			//�õ���ͼԪ������Ŀ
			GeoObjSpatial.MainGeometry.uiSubGeometryNum = pMultiPoint->getNumGeometries();

			//���յ���������ѭ����ȡ��ͼԪ����ռ���Ϣ
			BASIC_GEOMETRY SubGeometryTmp;	OGRPoint * pOGRPointTmp = NULL;
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				pOGRPointTmp = (OGRPoint *)pMultiPoint->getGeometryRef(i);

				SubGeometryTmp.GeometryType = SINGLE_POINT;
				SubGeometryTmp.uiNodePointNum = 1;
				SubGeometryTmp.NodePointList.clear();
				PT.dX = pOGRPointTmp->getX();
				PT.dY = pOGRPointTmp->getY();
				SubGeometryTmp.NodePointList.push_back(PT);
				SubGeometryTmp.uiRingNum = 0;
				SubGeometryTmp.Rings.clear();
				SubGeometryTmp.uiSubGeometryNum = 0;

				GeoObjSpatial.SubGeometrys.push_back(SubGeometryTmp);
			}

			OGRFeature::DestroyFeature(pFeature);

			return TRUE;
		}

		//�����
		else if(GeoObjSpatial.MainGeometry.GeometryType == MULTI_POLYLINES)
		{
			OGRMultiLineString * pMultiLines = (OGRMultiLineString *)pGeometry;

			//�õ���ͼԪ������Ŀ
			GeoObjSpatial.MainGeometry.uiSubGeometryNum = pMultiLines->getNumGeometries();

			//���յ���������ѭ����ȡÿ����ͼԪ����Ŀռ���Ϣ
			BASIC_GEOMETRY SubGeometryTmp;	OGRLineString * pOGRLineTmp = NULL;
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				pOGRLineTmp = (OGRLineString *)pMultiLines->getGeometryRef(i);

				SubGeometryTmp.GeometryType = SINGLE_POLYLINE;
				SubGeometryTmp.uiNodePointNum = pOGRLineTmp->getNumPoints();
				SubGeometryTmp.NodePointList.clear();
				for(unsigned int k=0; k<SubGeometryTmp.uiNodePointNum; k++)
				{
					PT.dX = pOGRLineTmp->getX(k);
					PT.dY = pOGRLineTmp->getY(k);
					SubGeometryTmp.NodePointList.push_back(PT);
				}
				SubGeometryTmp.uiRingNum = 0;
				SubGeometryTmp.Rings.clear();
				SubGeometryTmp.uiSubGeometryNum = 0;

				GeoObjSpatial.SubGeometrys.push_back(SubGeometryTmp);
			}

			OGRFeature::DestroyFeature(pFeature);

			return TRUE;
		}

		//��϶����
		else if(GeoObjSpatial.MainGeometry.GeometryType == MULTI_POLYGONS)
		{
			OGRMultiPolygon * pMultiPolygon = (OGRMultiPolygon *)pGeometry;

			//�õ���ͼԪ��������
			GeoObjSpatial.MainGeometry.uiSubGeometryNum = pMultiPolygon->getNumGeometries();

			//���յ������������ѭ����ȡÿ����ͼԪ����Ŀռ���Ϣ
			BASIC_GEOMETRY SubGeometryTmp;	OGRPolygon * pOGRPolygonTmp = NULL;	OGRLinearRing * pRing = NULL;	OGRLinearRing * pIslandRing = NULL;

			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				pOGRPolygonTmp = (OGRPolygon *)pMultiPolygon->getGeometryRef(i);

				SubGeometryTmp.GeometryType = SINGLE_POLYGON;

				//��ȡ������
				pRing = pOGRPolygonTmp->getExteriorRing();
				SubGeometryTmp.uiNodePointNum = pRing->getNumPoints();
				SubGeometryTmp.NodePointList.clear();
				for(unsigned int k=0; k<SubGeometryTmp.uiNodePointNum; k++)
				{
					PT.dX = pRing->getX(i);	PT.dY = pRing->getY(i);
					SubGeometryTmp.NodePointList.push_back(PT);
				}

				//��ȡ�ڶ����
				SubGeometryTmp.uiRingNum = pOGRPolygonTmp->getNumInteriorRings();
				for(unsigned int j=0; j<SubGeometryTmp.uiRingNum; j++)
				{
					pIslandRing = pOGRPolygonTmp->getInteriorRing(j);
					Ring.NodePointList.clear();
					Ring.uiNodePointNum = pIslandRing->getNumPoints();
					for(unsigned int k=0; k<Ring.uiNodePointNum; k++)
					{
						PT.dX = pIslandRing->getX(k);	PT.dY = pIslandRing->getY(k);
						Ring.NodePointList.push_back(PT);
					}
					SubGeometryTmp.Rings.push_back(Ring);
				}

				SubGeometryTmp.uiSubGeometryNum = 0;

				GeoObjSpatial.SubGeometrys.push_back(SubGeometryTmp);
			}

			OGRFeature::DestroyFeature(pFeature);

			return TRUE;
		}

		//���ͼԪ
		else if(GeoObjSpatial.MainGeometry.GeometryType == MULTI_GEOMETRYS)
		{
			//�ڶ���ռ������ ��������ռ��������Ҫ����������ʱ���ɣ�
			OGRGeometry * pGeometryTmp = NULL;		

			//�ڶ���ռ������ʱ����
			BASIC_GEOMETRY SubGeometry;

			//������ռ������ʱ����
			BASIC_GEOMETRY PartGeometry;

			//��ȡ��һ��(MainGeometry)
			OGRGeometryCollection * pCollection = (OGRGeometryCollection *)pGeometry;
			GeoObjSpatial.MainGeometry.uiSubGeometryNum = pCollection->getNumGeometries();			

			//ѭ����ȡ�ڶ���(SubGeometry)
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				//����ڶ���ռ����
				pGeometryTmp = (OGRGeometry *)pCollection->getGeometryRef(i);
				SubGeometry.GeometryType = (GEOMETRY_TYPE)wkbFlatten(pGeometryTmp->getGeometryType());
				SubGeometry.uiNodePointNum = 0;	SubGeometry.NodePointList.clear();
				SubGeometry.uiRingNum = 0;		SubGeometry.Rings.clear();
				//ע��ڶ�����Ӷ���������Ҫ�ھ����ȡ����������ʱ��ȡ,��Ҫ���ˣ�

				//������
				if(SubGeometry.GeometryType == SINGLE_POINT)
				{
					OGRPoint * pPoint = (OGRPoint *)pGeometryTmp;

					//���µڶ���ռ������Ӷ�����Ŀ
					SubGeometry.uiSubGeometryNum = 1;

					//��ȡ������ĵ���������
					{
						//����
						PartGeometry.GeometryType = SINGLE_POINT;

						//�⻷
						PartGeometry.uiNodePointNum = 1;
						PartGeometry.NodePointList.clear();
						PT.dX = pPoint->getX();
						PT.dY = pPoint->getY();				
						PartGeometry.NodePointList.push_back(PT);

						//�ڻ�
						PartGeometry.uiRingNum = 0;
						PartGeometry.Rings.clear();

						//�Ӷ�����
						PartGeometry.uiSubGeometryNum = 0;
					}
					GeoObjSpatial.PartGeometrys.push_back(PartGeometry);				
				}

				//������
				else if(SubGeometry.GeometryType == SINGLE_POLYLINE)
				{
					OGRLineString * pOGRLine = (OGRLineString *)pGeometryTmp;

					//���µڶ���ռ������Ӷ�����Ŀ
					SubGeometry.uiSubGeometryNum = 1;

					//��ȡ������ĵ���������
					{
						//����
						PartGeometry.GeometryType = SINGLE_POLYLINE;

						//�⻷
						PartGeometry.uiNodePointNum = pOGRLine->getNumPoints();
						PartGeometry.NodePointList.clear();
						for(unsigned int n=0; n<PartGeometry.uiNodePointNum; n++)
						{
							PT.dX = pOGRLine->getX(n);	PT.dY = pOGRLine->getY(n);
							PartGeometry.NodePointList.push_back(PT);
						}

						//�ڻ�
						PartGeometry.uiRingNum = 0;
						PartGeometry.Rings.clear();

						//�Ӷ�����
						PartGeometry.uiSubGeometryNum = 0;
					}
					GeoObjSpatial.PartGeometrys.push_back(PartGeometry);
				}

				//���������
				else if(SubGeometry.GeometryType == SINGLE_POLYGON)
				{
					OGRPolygon * pOGRPolygon = (OGRPolygon *)pGeometryTmp;

					//���µڶ���ռ������Ӷ�����Ŀ
					SubGeometry.uiSubGeometryNum = 1;

					//��ȡ������ĵ������������
					{
						//����
						PartGeometry.GeometryType = SINGLE_POLYGON;

						//�⻷
						OGRLinearRing * pRing = pOGRPolygon->getExteriorRing();
						PartGeometry.uiNodePointNum = pRing->getNumPoints();
						PartGeometry.NodePointList.clear();
						for(unsigned int n=0; n<PartGeometry.uiNodePointNum; n++)
						{
							PT.dX = pRing->getX(n);	PT.dY = pRing->getY(n);
							PartGeometry.NodePointList.push_back(PT);
						}

						//�ڻ�
						PartGeometry.uiRingNum = pOGRPolygon->getNumInteriorRings();
						PartGeometry.Rings.clear();
						for(unsigned int k=0; k<PartGeometry.uiRingNum; k++)
						{
							OGRLinearRing * pIslandRing = pOGRPolygon->getInteriorRing(k);					
							Ring.uiNodePointNum = pIslandRing->getNumPoints();
							Ring.NodePointList.clear();
							for(unsigned int n=0; n<Ring.uiNodePointNum; n++)
							{
								PT.dX = pIslandRing->getX(n);	PT.dY = pIslandRing->getY(n);
								Ring.NodePointList.push_back(PT);
							}
							PartGeometry.Rings.push_back(Ring);
						}

						//�Ӷ�����
						PartGeometry.uiSubGeometryNum = 0;
					}
					GeoObjSpatial.PartGeometrys.push_back(PartGeometry);
				}

				//��ϵ�
				else if(SubGeometry.GeometryType == MULTI_POINTS)
				{
					OGRMultiPoint * pMultiPoint = (OGRMultiPoint *)pGeometryTmp;

					//���µڶ���ռ������Ӷ�����Ŀ
					SubGeometry.uiSubGeometryNum = pMultiPoint->getNumGeometries();

					//��ȡ������Ķ��������
					for(unsigned int k=0; k<SubGeometry.uiSubGeometryNum; k++)
					{
						OGRPoint * pOGRPointTmp = (OGRPoint *)pMultiPoint->getGeometryRef(k);

						//����
						PartGeometry.GeometryType = SINGLE_POINT;

						//�⻷
						PartGeometry.uiNodePointNum = 1;
						PartGeometry.NodePointList.clear();
						PT.dX = pOGRPointTmp->getX();
						PT.dY = pOGRPointTmp->getY();
						PartGeometry.NodePointList.push_back(PT);

						//�ڻ�
						PartGeometry.uiRingNum = 0;
						PartGeometry.Rings.clear();

						//�Ӷ�����
						PartGeometry.uiSubGeometryNum = 0;

						GeoObjSpatial.PartGeometrys.push_back(PartGeometry);
					}
				}

				//�����
				else if(SubGeometry.GeometryType == MULTI_POLYLINES)
				{
					OGRMultiLineString * pMultiLines = (OGRMultiLineString *)pGeometryTmp;

					//���µڶ���ռ������Ӷ�����Ŀ
					SubGeometry.uiSubGeometryNum = pMultiLines->getNumGeometries();

					//��ȡ������Ķ��������
					for(unsigned int k=0; k<SubGeometry.uiSubGeometryNum; k++)
					{
						OGRLineString * pOGRLineTmp = (OGRLineString *)pMultiLines->getGeometryRef(k);

						//����
						PartGeometry.GeometryType = SINGLE_POLYLINE;

						//�⻷
						PartGeometry.uiNodePointNum = pOGRLineTmp->getNumPoints();
						PartGeometry.NodePointList.clear();
						for(unsigned int n=0; n<PartGeometry.uiNodePointNum; n++)
						{
							PT.dX = pOGRLineTmp->getX(n);
							PT.dY = pOGRLineTmp->getY(n);
							PartGeometry.NodePointList.push_back(PT);
						}

						//�ڻ�
						PartGeometry.uiRingNum = 0;
						PartGeometry.Rings.clear();

						//�Ӷ�����
						PartGeometry.uiSubGeometryNum = 0;

						GeoObjSpatial.PartGeometrys.push_back(PartGeometry);
					}	
				}

				//��϶����
				else if(SubGeometry.GeometryType == MULTI_POLYGONS)
				{
					OGRMultiPolygon * pMultiPolygon = (OGRMultiPolygon *)pGeometryTmp;

					//���µڶ���ռ������Ӷ�����Ŀ
					SubGeometry.uiSubGeometryNum = pMultiPolygon->getNumGeometries();

					//��ȡ������Ķ�����������
					for(unsigned int k=0; k<SubGeometry.uiSubGeometryNum; k++)
					{
						OGRPolygon * pOGRPolygonTmp = (OGRPolygon *)pMultiPolygon->getGeometryRef(k);

						//����
						PartGeometry.GeometryType = SINGLE_POLYGON;

						//�⻷
						pRing = pOGRPolygonTmp->getExteriorRing();
						PartGeometry.uiNodePointNum = pRing->getNumPoints();
						PartGeometry.NodePointList.clear();
						for(unsigned int n=0; n<PartGeometry.uiNodePointNum; n++)
						{
							PT.dX = pRing->getX(n);	PT.dY = pRing->getY(n);
							PartGeometry.NodePointList.push_back(PT);
						}

						//�ڻ�
						PartGeometry.uiRingNum = pOGRPolygonTmp->getNumInteriorRings();
						for(unsigned int j=0; j<PartGeometry.uiRingNum; j++)
						{
							pIslandRing = pOGRPolygonTmp->getInteriorRing(j);
							Ring.uiNodePointNum = pIslandRing->getNumPoints();
							Ring.NodePointList.clear();
							for(unsigned int n=0; n<Ring.uiNodePointNum; n++)
							{
								PT.dX = pIslandRing->getX(n);	PT.dY = pIslandRing->getY(n);
								Ring.NodePointList.push_back(PT);
							}
							PartGeometry.Rings.push_back(Ring);
						}

						//�Ӷ�����
						PartGeometry.uiSubGeometryNum = 0;

						GeoObjSpatial.PartGeometrys.push_back(PartGeometry);
					}

				}

				//����δ֪���ʹ���
				else
				{
					CString str;str.Format("GEOMETRY_TYPE Error: %d @ ReadSpatialInfo - MULTI_GEOMETRYS",SubGeometry.GeometryType);
					strErrMsgs.push_back(str);
					return FALSE;
				}

				//���µڶ���ռ��������
				GeoObjSpatial.SubGeometrys.push_back(SubGeometry);
			}

			OGRFeature::DestroyFeature(pFeature);

			return TRUE;
		}

		//����δ֪���ʹ���
		else
		{
			strErrMsgs.push_back("UNKNOWN_GEOMETRY_TYPE @ ReadSpatialInfo");

			OGRFeature::DestroyFeature(pFeature);

			return FALSE;
		}
	}

	OGRFeature::DestroyFeature(pFeature);

	return TRUE;
}





BOOL CVectorIO::WriteGeoObject(INT64 i64FeatureNo, GEO_OBJECT_SPATIAL GeoObjSpatial,GEO_OBJECT_ATTRIBUTE GeoObjAttribute, BOOL bUpdateSpatialInfo, BOOL bUpdateAttributeInfo, BOOL bCreateNewFeature, int iLayerNo /* = 0 */)
{
	strErrMsgs.clear();

	//��ȫ���: ����ļ�δ��, ��ֱ���˳�
	if(BasicInfo.iFileStatus < 0)
	{
		CString str;	str.Format("iFileStatus = %d @ ReadAttributeInfo",BasicInfo.iFileStatus);
		strErrMsgs.push_back(str);
		return FALSE;
	}

	//��ȡͼ����
	OGRLayer * pLayer = NULL;
	pLayer = pDataset->GetLayer(iLayerNo);
	if(pLayer == NULL)
	{
		strErrMsgs.push_back("pLyaer == NULL @ WriteSpatialInfo");
		return FALSE;
	}

	//��ȡ����������
	OGRFeature * pFeature = NULL;

	//�½�����
	if(bCreateNewFeature == TRUE)
	{
		pFeature = OGRFeature::CreateFeature(pLayer->GetLayerDefn());
	}

	//�������ж���
	else
	{
		//��ȫ���: �ж�i64FeatureNo�Ƿ�Ϸ�
		if(i64FeatureNo < 0 || i64FeatureNo > BasicInfo.i64FeatureNum[iLayerNo])
		{
			CString str;
			str.Format("i64FeatureNo[%d] / TotalFeatureNum[%d] Error! @ WriteGeoObject",i64FeatureNo,BasicInfo.i64FeatureNum[iLayerNo]);
			strErrMsgs.push_back(str);
			return FALSE;
		}

		//��ȫ���: �жϵ�ǰͼ���Ƿ�֧�������д����ָ�������Ž��и��£�
		if(TRUE != pLayer->TestCapability(OLCRandomWrite))
		{
			strErrMsgs.push_back("pLayer is not support RandomWrite @ WriteGeoObject");
			return FALSE;
		}

		//���⴦��: ���ΪMapInfo�ļ�,�ռ�����±��1��ʼ,������0��ʼ
		if(BasicInfo.enFileType == MAPINFO_MIF_FILE || BasicInfo.enFileType == MAPINFO_TAB_FILE)
		{
			i64FeatureNo++;
		}

		//��ȡ��ǰ�����ռ������
		pFeature = pLayer->GetFeature(i64FeatureNo);
	}

	if(pFeature == NULL)
	{
		strErrMsgs.push_back("pFeature = NULL @ WriteGeoObject");
		return FALSE;
	}

	//���¿ռ�����������Ϣ
	if(bUpdateAttributeInfo == TRUE)
	{
		//��ȫ���
		if(int(GeoObjAttribute.uiFieldNum) < BasicInfo.Attribute[iLayerNo].iFieldNum || int(GeoObjAttribute.FieldInfoString.size()) < BasicInfo.Attribute[iLayerNo].iFieldNum)
		{
			CString str;
			str.Format("GeoObjAttribute.uiFieldNum[%d] is less than BasicInfo.Attribute[%d].iFieldNum[%d]",GeoObjAttribute.uiFieldNum,iLayerNo,BasicInfo.Attribute[iLayerNo].iFieldNum);
			strErrMsgs.push_back(str);
			return FALSE;
		}

		//��ӿռ�����������Ϣ
		for(int i=0; i<BasicInfo.Attribute[iLayerNo].iFieldNum; i++)
		{
			pFeature->SetField(BasicInfo.Attribute[iLayerNo].strFieldName[i],GeoObjAttribute.FieldInfoString[i]);
		}
	}

	//���¶���Ŀռ���Ϣ
	if(bUpdateSpatialInfo == TRUE)
	{
		//���ݲ�ͬ��ͼԪ������ӿռ����Ŀռ���Ϣ
		OGRPoint PointTmp;	
		OGRLineString LineTmp;	
		OGRLinearRing RingTmp; 
		OGRPolygon PolygonTmp;	
		OGRMultiPoint MultiPointTmp;
		OGRMultiLineString MultiLineTmp;
		OGRMultiPolygon MultiPolygonTmp;
		OGRGeometryCollection MultiCollectionTmp;
		unsigned int uiPartGeometryPointer = 0;

		switch(GeoObjSpatial.MainGeometry.GeometryType)
		{
			//������
		case SINGLE_POINT:
			//��ȫ���
			if(BasicInfo.enFileType == ESRI_SHAPE_FILE && BasicInfo.GeometryType[iLayerNo] != wkbPoint)
			{
				strErrMsgs.push_back("FILE TYPE & GEOMETRY TYPE OF LAYER IS NOT MATCH! @ WriteGeoObject - SINGLE_POINT");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			if(GeoObjSpatial.MainGeometry.uiNodePointNum < 1 || GeoObjSpatial.MainGeometry.NodePointList.size() < 1)
			{
				strErrMsgs.push_back("GeoObjSpatial.MainGeometry.uiNodePointNum is Less than 1 @ WriteGeoObject - SINGLE_POINT");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}

			//��ȡ��������Ϣ
			PointTmp.empty();
			PointTmp.setX(GeoObjSpatial.MainGeometry.NodePointList[0].dX);
			PointTmp.setY(GeoObjSpatial.MainGeometry.NodePointList[0].dY);

			//���ÿռ����
			if(pFeature->SetGeometry(&PointTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - SINGLE_POINT");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}			
			break;

			//������
		case SINGLE_POLYLINE:
			//��ȫ���
			if(BasicInfo.enFileType == ESRI_SHAPE_FILE && BasicInfo.GeometryType[iLayerNo] != wkbLineString)
			{
				strErrMsgs.push_back("FILE TYPE & GEOMETRY TYPE OF LAYER IS NOT MATCH! @ WriteGeoObject - SINGLE_POLYLINE");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			if(GeoObjSpatial.MainGeometry.uiNodePointNum < 2 || GeoObjSpatial.MainGeometry.NodePointList.size() < 2)
			{
				strErrMsgs.push_back("GeoObjSpatial.MainGeometry.uiNodePointNum is less than 2! @ WriteGeoObject - SINGLE_POLYLINE");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}

			//��ȡ��������Ϣ
			LineTmp.empty();
			for(unsigned int n=0; n<GeoObjSpatial.MainGeometry.uiNodePointNum; n++)
			{
				LineTmp.addPoint(GeoObjSpatial.MainGeometry.NodePointList[n].dX,GeoObjSpatial.MainGeometry.NodePointList[n].dY);
			}

			//���ÿռ����
			if(pFeature->SetGeometry(&LineTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - SINGLE_POLYLINE");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}		
			break;


			//���������
		case SINGLE_POLYGON:
			//��ȫ���
			if(BasicInfo.enFileType == ESRI_SHAPE_FILE && BasicInfo.GeometryType[iLayerNo] != wkbPolygon)
			{
				strErrMsgs.push_back("FILE TYPE & GEOMETRY TYPE OF LAYER IS NOT MATCH! @ WriteGeoObject - SINGLE_POLYGON");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			if(GeoObjSpatial.MainGeometry.uiNodePointNum < 3 || GeoObjSpatial.MainGeometry.NodePointList.size() < 3)
			{
				strErrMsgs.push_back("GeoObjSpatial.MainGeometry.uiNodePointNum is less than 3! @ WriteGeoObject - SINGLE_POLYGON");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}

			//��ӿռ���Ϣ
			PolygonTmp.empty();	RingTmp.empty();
			//�⻷
			for(unsigned int n=0; n<GeoObjSpatial.MainGeometry.uiNodePointNum; n++)
			{
				RingTmp.addPoint(GeoObjSpatial.MainGeometry.NodePointList[n].dX, GeoObjSpatial.MainGeometry.NodePointList[n].dY);
			}
			RingTmp.closeRings();
			PolygonTmp.addRing(&RingTmp);

			//�ڻ�
			for(unsigned int j=0; j<GeoObjSpatial.MainGeometry.uiRingNum; j++)
			{
				RingTmp.empty();
				//��ȫ���
				if(GeoObjSpatial.MainGeometry.Rings[j].uiNodePointNum < 3 || GeoObjSpatial.MainGeometry.Rings[j].NodePointList.size() < 3)
				{
					CString str;
					str.Format("GeoObjSpatial.Ring[%d].uiNodePointNum is less than 3! @ WriteGeoObject - SINGLE_POLYGON",j);
					strErrMsgs.push_back(str);
					OGRFeature::DestroyFeature(pFeature);
					return FALSE;
				}
				//��ӿռ���Ϣ
				for(unsigned int n=0; n<GeoObjSpatial.MainGeometry.Rings[j].uiNodePointNum; n++)
				{
					RingTmp.addPoint(GeoObjSpatial.MainGeometry.Rings[j].NodePointList[n].dX, GeoObjSpatial.MainGeometry.Rings[j].NodePointList[n].dY);
				}
				RingTmp.closeRings();
				PolygonTmp.addRing(&RingTmp);
			}

			//���ÿռ����
			if(pFeature->SetGeometry(&PolygonTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - SINGLE_POLYGON");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			break;

			//��ϵ�
		case MULTI_POINTS:
			//��ȫ���
			if(BasicInfo.enFileType == ESRI_SHAPE_FILE && BasicInfo.GeometryType[iLayerNo] != wkbPoint)
			{
				strErrMsgs.push_back("FILE TYPE & GEOMETRY TYPE OF LAYER IS NOT MATCH! @ WriteGeoObject - MULTI_POINTS");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}

			//��ӿռ���Ϣ
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				//��ȫ���
				if(GeoObjSpatial.SubGeometrys[i].uiNodePointNum < 1 || GeoObjSpatial.SubGeometrys[i].NodePointList.size() < 1)
				{
					CString str;
					str.Format("GeoObjSpatial.SubGeometrys[%d].uiNodePointNum is less than 1 @ WriteGeoObject - MULTI_POINTS",i);
					strErrMsgs.push_back(str);
					OGRFeature::DestroyFeature(pFeature);
					return FALSE;
				}

				//��ӿռ���Ϣ
				PointTmp.empty();
				PointTmp.setX(GeoObjSpatial.SubGeometrys[i].NodePointList[0].dX);
				PointTmp.setY(GeoObjSpatial.SubGeometrys[i].NodePointList[0].dY);

				MultiPointTmp.addGeometry(&PointTmp);
			}

			//���ÿռ����
			if(pFeature->SetGeometry(&MultiPointTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - MULTI_POINTS");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			break;

			//�����
		case MULTI_POLYLINES:
			//��ȫ���
			if(BasicInfo.enFileType == ESRI_SHAPE_FILE && BasicInfo.GeometryType[iLayerNo] != wkbLineString)
			{
				strErrMsgs.push_back("FILE TYPE & GEOMETRY TYPE OF LAYER IS NOT MATCH! @ WriteGeoObject - MULTI_POLYLINES");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}

			//��ӿռ���Ϣ
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				//��ȫ���
				if(GeoObjSpatial.SubGeometrys[i].uiNodePointNum < 2 || GeoObjSpatial.SubGeometrys[i].NodePointList.size() < 2)
				{
					CString str;
					str.Format("GeoObjSpatial.SubGeometrys[%d].uiNodePointNum is less than 2! @ WriteGeoObject - MULTI_POLYLINES",i);
					strErrMsgs.push_back(str);
					OGRFeature::DestroyFeature(pFeature);
					return FALSE;
				}

				//��ӿռ���Ϣ
				LineTmp.empty();
				for(unsigned int n=0; n<GeoObjSpatial.SubGeometrys[i].uiNodePointNum; n++)
				{
					LineTmp.addPoint(GeoObjSpatial.SubGeometrys[i].NodePointList[n].dX, GeoObjSpatial.SubGeometrys[i].NodePointList[n].dY);
				}

				MultiLineTmp.addGeometry(&LineTmp);
			}

			//���ÿռ����
			if(pFeature->SetGeometry(&MultiLineTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - MULTI_POLYLINES");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			break;

			//��϶����
		case MULTI_POLYGONS:
			//��ȫ���
			if(BasicInfo.enFileType == ESRI_SHAPE_FILE && BasicInfo.GeometryType[iLayerNo] != wkbPolygon)
			{
				strErrMsgs.push_back("FILE TYPE & GEOMETRY TYPE OF LAYER IS NOT MATCH! @ WriteGeoObject - MULTI_POLYGONS");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}

			//��ӿռ���Ϣ
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				PolygonTmp.empty();

				//��ȫ���
				if(GeoObjSpatial.SubGeometrys[i].uiNodePointNum < 3 || GeoObjSpatial.SubGeometrys[i].NodePointList.size() < 3)
				{
					CString str;
					str.Format("GeoObjSpatial.SubGeometrys[%d].uiNodePointNum is less than 3! @ WriteGeoObject - MULTI_POLYGONS",i);
					strErrMsgs.push_back(str);
					OGRFeature::DestroyFeature(pFeature);
					return FALSE;
				}

				//��ӿռ���Ϣ					
				//�⻷
				RingTmp.empty();
				for(unsigned int n=0; n<GeoObjSpatial.SubGeometrys[i].uiNodePointNum; n++)
				{
					RingTmp.addPoint(GeoObjSpatial.SubGeometrys[i].NodePointList[n].dX, GeoObjSpatial.SubGeometrys[i].NodePointList[n].dY);
				}
				RingTmp.closeRings();
				PolygonTmp.addRing(&RingTmp);

				//�ڻ�
				for(unsigned int j=0; j<GeoObjSpatial.SubGeometrys[i].uiRingNum; j++)
				{
					//��ȫ���
					if(GeoObjSpatial.SubGeometrys[i].Rings[j].uiNodePointNum < 3 || GeoObjSpatial.SubGeometrys[i].Rings[j].NodePointList.size() < 3)
					{
						CString str;
						str.Format("GeoObjSpatial.SubGeometrys[%d].Rings[%d].uiNodePointNum is less than 3! @ WriteGeoObject - MULTI_POLYGONS",i,j);
						strErrMsgs.push_back(str);
						OGRFeature::DestroyFeature(pFeature);
						return FALSE;
					}
					//��ӿռ���Ϣ
					RingTmp.empty();
					for(unsigned int n=0; n<GeoObjSpatial.SubGeometrys[i].Rings[j].uiNodePointNum; n++)
					{
						RingTmp.addPoint(GeoObjSpatial.SubGeometrys[i].Rings[j].NodePointList[n].dX, GeoObjSpatial.SubGeometrys[i].Rings[j].NodePointList[n].dY);
					}
					RingTmp.closeRings();
					PolygonTmp.addRing(&RingTmp);
				}

				MultiPolygonTmp.addGeometry(&PolygonTmp);
			}

			//���ÿռ����
			if(pFeature->SetGeometry(&MultiPolygonTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - MULTI_POLYGONS");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			break;

			//���ͼԪ
		case MULTI_GEOMETRYS:
			//��ȫ���
			if(BasicInfo.enFileType == ESRI_SHAPE_FILE)
			{
				strErrMsgs.push_back("FILE TYPE IS NOT SUPPORT THIS GEOMETRY TYPE! @ WriteGeoObject - MULTI_GEOMETRYS");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}

			//��ӿռ���Ϣ
			uiPartGeometryPointer = 0;
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				//������
				if(GeoObjSpatial.SubGeometrys[i].GeometryType == SINGLE_POINT)
				{
					//��ȫ���
					if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].uiNodePointNum < 1 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList.size() < 1)
					{
						CString str;
						str.Format("GeoObjSpatial.PartGeometrys[%d].uiNodePointNum is less than 1 @ WriteGeoObject - MULTI_GEOMTRYS - SINGLE_POINT",uiPartGeometryPointer);
						strErrMsgs.push_back(str);
						OGRFeature::DestroyFeature(pFeature);
						return FALSE;
					}

					//�⻷
					PointTmp.empty();
					PointTmp.setX(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList[0].dX);
					PointTmp.setY(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList[0].dY);
					MultiCollectionTmp.addGeometry(&PointTmp);

					//�ƶ�������ռ��������ָ��
					uiPartGeometryPointer++;
				}

				//������
				else if(GeoObjSpatial.SubGeometrys[i].GeometryType == SINGLE_POLYLINE)
				{
					//��ȫ���
					if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].uiNodePointNum < 2 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList.size() < 2)
					{
						CString str;
						str.Format("GeoObjSpatial.PartGeometrys[%d].uiNodePointNum is less than 2 @ WriteGeoObject - MULTI_GEOMTRYS - SINGLE_POLYLINE",uiPartGeometryPointer);
						strErrMsgs.push_back(str);
						OGRFeature::DestroyFeature(pFeature);
						return FALSE;
					}

					//�⻷
					LineTmp.empty();
					for(unsigned int n=0; n<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].uiNodePointNum; n++)
					{
						LineTmp.addPoint(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList[n].dX, GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList[n].dY);
					}
					MultiCollectionTmp.addGeometry(&LineTmp);

					//�ƶ�������ռ��������ָ��
					uiPartGeometryPointer++;
				}

				//���������
				else if(GeoObjSpatial.SubGeometrys[i].GeometryType == SINGLE_POLYGON)
				{
					//��ȫ���
					if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].uiNodePointNum < 3 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList.size() < 3)
					{
						CString str;
						str.Format("GeoObjSpatial.PartGeometrys[%d].uiNodePointNum is less than 3 @ WriteGeoObject - MULTI_GEOMTRYS - SINGLE_POLYGON",uiPartGeometryPointer);
						strErrMsgs.push_back(str);
						OGRFeature::DestroyFeature(pFeature);
						return FALSE;
					}

					//�⻷
					PolygonTmp.empty();	RingTmp.empty();
					for(unsigned int n=0; n<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].uiNodePointNum; n++)
					{
						RingTmp.addPoint(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList[n].dX, GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList[n].dY);
					}
					RingTmp.closeRings();
					PolygonTmp.addRing(&RingTmp);

					//�ڻ�
					for(unsigned int j=0; j<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].uiRingNum; j++)
					{
						//��ȫ���
						if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].Rings[j].uiNodePointNum < 3 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].Rings[j].NodePointList.size() < 3)
						{
							CString str;
							str.Format("GeoObjSpatial.PartGeometrys[%d].Rings[%d].uiNodePointNum is less than 3 @ WriteGeoObject - MULTI_GEOMTRYS - SINGLE_POLYGON",uiPartGeometryPointer,j);
							strErrMsgs.push_back(str);
							OGRFeature::DestroyFeature(pFeature);
							return FALSE;
						}
						RingTmp.empty();
						for(unsigned int n=0; n<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].Rings[j].uiNodePointNum; n++)
						{
							RingTmp.addPoint(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].Rings[j].NodePointList[n].dX, GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].Rings[j].NodePointList[n].dY);
						}
						RingTmp.closeRings();
						PolygonTmp.addRing(&RingTmp);
					}

					MultiCollectionTmp.addGeometry(&PolygonTmp);

					//�ƶ�������ռ��������ָ��
					uiPartGeometryPointer++;
				}

				//�����
				else if(GeoObjSpatial.SubGeometrys[i].GeometryType == MULTI_POINTS)
				{
					MultiPointTmp.empty();

					for(unsigned int k=0; k<GeoObjSpatial.SubGeometrys[i].uiSubGeometryNum; k++)
					{
						//��ȫ���
						if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].uiNodePointNum < 1 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList.size() < 1)
						{
							CString str;
							str.Format("GeoObjSpatial.PartGeometrys[%d].uiNodePointNum is less than 1 @ WriteGeoObject - MULTI_GEOMTRYS - MULTI_POINTS",uiPartGeometryPointer + k);
							strErrMsgs.push_back(str);
							OGRFeature::DestroyFeature(pFeature);
							return FALSE;
						}

						//�⻷
						PointTmp.empty();
						PointTmp.setX(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList[0].dX);
						PointTmp.setY(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList[0].dY);
						MultiPointTmp.addGeometry(&PointTmp);
					}

					MultiCollectionTmp.addGeometry(&MultiPointTmp);

					//�ƶ�������ռ��������ָ��
					uiPartGeometryPointer += GeoObjSpatial.SubGeometrys[i].uiSubGeometryNum;
				}

				//�����
				else if(GeoObjSpatial.SubGeometrys[i].GeometryType == MULTI_POLYLINES)
				{
					MultiLineTmp.empty();

					for(unsigned int k=0; k<GeoObjSpatial.SubGeometrys[i].uiSubGeometryNum; k++)
					{
						//��ȫ���
						if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].uiNodePointNum < 2 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList.size() < 2)
						{
							CString str;
							str.Format("GeoObjSpatial.PartGeometrys[%d].uiNodePointNum is less than 2 @ WriteGeoObject - MULTI_GEOMTRYS - MULTI_POLYLINES",uiPartGeometryPointer + k);
							strErrMsgs.push_back(str);
							OGRFeature::DestroyFeature(pFeature);
							return FALSE;
						}

						//�⻷
						LineTmp.empty();

						for(unsigned int n=0; n<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].uiNodePointNum; n++)
						{
							LineTmp.addPoint(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList[n].dX, GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList[n].dY);
						}

						MultiLineTmp.addGeometry(&LineTmp);
					}

					MultiCollectionTmp.addGeometry(&MultiLineTmp);

					//�ƶ�������ռ��������ָ��
					uiPartGeometryPointer += GeoObjSpatial.SubGeometrys[i].uiSubGeometryNum;
				}

				//��������
				else if(GeoObjSpatial.SubGeometrys[i].GeometryType == MULTI_POLYGONS)
				{

					MultiPolygonTmp.empty();

					for(unsigned int k=0; k<GeoObjSpatial.SubGeometrys[i].uiSubGeometryNum; k++)
					{
						//��ȫ���
						if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].uiNodePointNum < 3 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList.size() < 3)
						{
							CString str;
							str.Format("GeoObjSpatial.PartGeometrys[%d].uiNodePointNum is less than 3 @ WriteGeoObject - MULTI_GEOMTRYS - MULTI_POLYGONS",uiPartGeometryPointer + k);
							strErrMsgs.push_back(str);
							OGRFeature::DestroyFeature(pFeature);
							return FALSE;
						}

						//�⻷
						PolygonTmp.empty();		RingTmp.empty();
						for(unsigned int n=0; n<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].uiNodePointNum; n++)
						{
							RingTmp.addPoint(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList[n].dX, GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList[n].dY);
						}
						RingTmp.closeRings();
						PolygonTmp.addRing(&RingTmp);

						//�ڻ�
						for(unsigned int j=0; j<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].uiRingNum; j++)
						{
							//��ȫ���
							if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].Rings[j].uiNodePointNum < 3 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList.size() < 3)
							{
								CString str;
								str.Format("GeoObjSpatial.PartGeometrys[%d].Rings[%d].uiNodePointNum is less than 3 @ WriteGeoObject - MULTI_GEOMTRYS - MULTI_POLYLINES",uiPartGeometryPointer + k,j);
								strErrMsgs.push_back(str);
								OGRFeature::DestroyFeature(pFeature);
								return FALSE;
							}

							RingTmp.empty();
							for(unsigned int n=0; n<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].Rings[j].uiNodePointNum; n++)
							{
								RingTmp.addPoint(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].Rings[j].NodePointList[n].dX, GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].Rings[j].NodePointList[n].dY);
							}
							RingTmp.closeRings();
							PolygonTmp.addRing(&RingTmp);
						}

						MultiPolygonTmp.addGeometry(&PolygonTmp);
					}

					MultiCollectionTmp.addGeometry(&MultiPolygonTmp);

					//�ƶ�������ռ��������ָ��
					uiPartGeometryPointer += GeoObjSpatial.SubGeometrys[i].uiSubGeometryNum;
				}

				else
				{
					CString str;
					str.Format("GEOMETRY TYPE ERROR: GeoObjSaptial.SubGeometrys[%d] @  WriteGeoObject - MULTI_GEOMETRYS",i);
					strErrMsgs.push_back(str);
					OGRFeature::DestroyFeature(pFeature);
					return FALSE;
				}
			}

			//���ÿռ����
			if(pFeature->SetGeometry(&MultiCollectionTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - MULTI_GEOMETRYS");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			break;

			//δ֪���ʹ���
		default:
			CString str;
			str.Format("GeoObjSpatial.MainGeometry.GeometryType Error: %d, @  WriteGeoObject",GeoObjSpatial.MainGeometry.GeometryType);
			strErrMsgs.push_back(str);
			OGRFeature::DestroyFeature(pFeature);
			return FALSE;
		}

		//�ͷ�����
		PointTmp.empty();		LineTmp.empty();		RingTmp.empty(); 			PolygonTmp.empty();	
		MultiPointTmp.empty();	MultiLineTmp.empty();	MultiPolygonTmp.empty();	MultiCollectionTmp.empty();
	}

	//�ڵ�ǰ����ͼ�����½�����¿ռ����
	OGRErr err;

	if(bCreateNewFeature == TRUE)
	{
		err = pLayer->CreateFeature(pFeature);
	}
	else
	{
		err = pLayer->SetFeature(pFeature);
	}

	OGRFeature::DestroyFeature(pFeature);

	if(err != OGRERR_NONE)
	{
		strErrMsgs.push_back("CreateFeature Or SetFeature Error! @  WriteGeoObject");
		return FALSE;
	}
	else
	{		
		return TRUE;
	}

}

BOOL CVectorIO::DeleteGeoObject(INT64 i64FeatureNo, int iLayerNo /* = 0 */)
{
	strErrMsgs.clear();

	//��ȫ��飺����ļ�δ��, ��ֱ���˳�
	if(BasicInfo.iFileStatus < 0)
	{
		CString str;	str.Format("iFileStatus = %d @ DeleteGeoObject",BasicInfo.iFileStatus);
		strErrMsgs.push_back(str);
		return FALSE;
	}

	//��ȫ��飺���ָ��iLayerNo������ͼ�����򲻺Ϸ�, ��ֱ���˳�
	if(iLayerNo >= BasicInfo.iLayerNum || iLayerNo < 0)
	{
		CString str;	str.Format("iLayerNo = %d @ DeleteGeoObject",iLayerNo);
		strErrMsgs.push_back(str);
		return FALSE;
	}

	//��ȫ��飺���ָ��i64FeatureNo�����ܶ������򲻺Ϸ�, ��ֱ���˳�
	//Mapinfo�Ŀռ�����±��1��ʼ,Ϊ��ͳһ����,�ڴ����⴦��һ��
	if(BasicInfo.enFileType == MAPINFO_MIF_FILE || BasicInfo.enFileType == MAPINFO_TAB_FILE)
	{
		i64FeatureNo++;
	}
	if(i64FeatureNo > BasicInfo.i64FeatureNum[iLayerNo] || i64FeatureNo < 0)
	{
		CString str;	str.Format("i64FeatureNo = %d @ DeleteGeoObject",i64FeatureNo);
		strErrMsgs.push_back(str);
		return FALSE;
	}

	//��ȡ����ͼ����
	OGRLayer * pLayer = NULL;
	pLayer = pDataset->GetLayer(iLayerNo);
	if(pLayer == NULL)
	{
		strErrMsgs.push_back("pLayer = NULL @ DeleteGeoObject");
		return FALSE;
	}

	//��ȫ���:�жϵ�ǰ����ͼ���Ƿ�֧��ɾ���ռ����
	if(TRUE != pLayer->TestCapability(OLCDeleteFeature))
	{
		strErrMsgs.push_back("pLayer is not support RandomWrite @ WriteGeoObject");
		return FALSE;
	}

	//ɾ���ռ����DeleteFeature���ܱ�Ǹö�����Ҫ��ɾ����
	OGRErr err = pLayer->DeleteFeature(i64FeatureNo);

	if(err != OGRERR_NONE)
	{
		CString str;
		str.Format("DeleteFeature Error: %d",err);
		strErrMsgs.push_back(str);
		return FALSE;
	}
	else
	{
		//DeleteFeatureֻ�������ݼ������˱�ǣ�����ɾ����Ҫ��CloseFileʱ����PACK������
		bDeleteFeature = TRUE;

		return TRUE;
	}
}
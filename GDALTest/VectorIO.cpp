#include "StdAfx.h"
#include "VectorIO.h"

CString CVectorIO::ConvertTextInCoding(const char * pText, int iCodingCur)
{
	CString strRet;

	//不转码
	if(iCodingCur == -1)
	{
		strRet = pText;
	}

	//UTF-8转码
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
	//查找是否有CPG文件
	CFileFind finder;
	BOOL bRet = finder.FindFile(strName.Left(strName.GetLength() - 3) + "cpg");

	if(!bRet)
		return FALSE;

	//如果存在则打开CPG文件
	CStdioFile File;
	CString CPGFile = strName.Left(strName.GetLength() - 3) + "cpg";
	if(!File.Open(CPGFile,CFile::modeRead | CFile::typeText,NULL))
	{
		AfxMessageBox("Open CPG File Error! @ CVectorIO::GetTextCodingFromFileForShape");
		return FALSE;
	}

	//读取编码方式字符串
	CString strLine;
	File.ReadString(strLine);

	if(strLine == "")
	{
		//不用管
	}
	if(strLine == "UTF-8")
	{
		iCoding = 0;	//指定编码方式为强制UTF-8转换
	}
	else
	{
		AfxMessageBox("Cannot tell CPF File!");	//报告一下新的编码方式,以便进一步补充完善此类
	}

	return TRUE;
}

CVectorIO::CVectorIO(void)
{
	//注册驱动
	GDALAllRegister();

	//OGRRegisterAll(); 

	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8","NO");
	CPLSetConfigOption("SHAPE_ENCODING","");

	pDataset = NULL;

	bErrLogOpen = FALSE;

	bDeleteFeature = FALSE;

	InitDataVessel();

	//---初始化文件创建选项列表
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

	//默认不进行特殊强制转换
	iCoding = -1;

	//开辟一个公用字符转换空间
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
	//关闭句柄
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
			//TAB文件需要新建
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

			//替换文件
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

	//清空数据容器
	if(BasicInfo.iFileStatus != -1)
	{
		InitDataVessel();
	}

	//获取文件路径、扩展名、标记文件类型
	strFilePath = strFileName;	strFileExt = strFileName.Right(3);	strFileExt.MakeUpper();

	if(strFileExt == "SHP")
	{
		BasicInfo.enFileType = ESRI_SHAPE_FILE;
	}
	else if(strFileExt == "TAB"){BasicInfo.enFileType = MAPINFO_TAB_FILE;}
	else if(strFileExt == "MIF"){BasicInfo.enFileType = MAPINFO_MIF_FILE;}
	else{strErrMsgs.push_back("Not Support Now! @ CVectorIO::OpenFile");	return FALSE;}

	//MIF文件不支持以UPDATE方式打开
	if(BasicInfo.enFileType == MAPINFO_MIF_FILE)
	{
		pDataset = (GDALDataset *)GDALOpenEx(strFileName,GDAL_OF_VECTOR,NULL,NULL,NULL);
	}
	else
	{
		pDataset = (GDALDataset *)GDALOpenEx(strFileName,GDAL_OF_VECTOR | GDAL_OF_UPDATE,NULL,NULL,NULL);

		//更新编码转换标识
		iCoding = iForceCoding;

		//查找是否有CPG文件标识出编码
		GetTextCodingFromFileForShape(strFileName);
	}

	if(pDataset == NULL){strErrMsgs.push_back("pDataset == NULL; @ CVectorIO::OpenFile");	return FALSE;}

	//获取图层数
	BasicInfo.iLayerNum = pDataset->GetLayerCount();

	OGRLayer * pLayer = NULL;
	OGRFeatureDefn * pAttribute = NULL;
	OGRFieldDefn * pField = NULL;

	ATTRIBUTE_INFO AttributeInfo;

	//获取各图层记录数
	for(int i=0; i<BasicInfo.iLayerNum; i++)
	{
		//获取当前操作图层句柄
		pLayer = pDataset->GetLayer(i);
		if(pLayer == NULL){strErrMsgs.push_back("pLayer == NULL @ CVectorIO::OpenFile");	return FALSE;}

		//获取空间范围（只考虑第一层）
		if(i==0)
		{
			OGREnvelope Envelope;
			pDataset->GetLayer(0)->GetExtent(&Envelope);
			BasicInfo.dBounds[0] = Envelope.MinX;	BasicInfo.dBounds[1] = Envelope.MinY;	
			BasicInfo.dBounds[2] = Envelope.MaxX;	BasicInfo.dBounds[3] = Envelope.MaxY;
		}

		//获取该图层名称
		BasicInfo.strLayerName.push_back(pLayer->GetName());

		//获取空间参考
		OGRSpatialReference * SpatialReference = NULL;
		SpatialReference = pLayer->GetSpatialRef();
		if(SpatialReference != NULL)
			BasicInfo.SpatialRef.push_back(*SpatialReference);

		//获取该图层图元类型
		BasicInfo.GeometryType.push_back(pLayer->GetGeomType());

		//获取该图层空间对象数目
		//如果为0的话,需要考虑该图层没有被REPACK的情况,因此再通过遍历的方法统计一下对象数
		INT64 iFeatureNum = pLayer->GetFeatureCount();
		//if(iFeatureNum <= 0)
		iFeatureNum = GetFeatureNum(pLayer);

		BasicInfo.i64FeatureNum.push_back(iFeatureNum);

		//获取该图层属性表
		pAttribute = pLayer->GetLayerDefn();
		if(pAttribute == NULL){strErrMsgs.push_back("pAttribute == NULL @ CVectorIO::OpenFile");	return FALSE;}

		//解析属性表并保存至基本信息结构体
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

	//标记文件打开状态
	BasicInfo.iFileStatus = 0;

	//导出基本信息
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

	//清空数据容器
	if(BasicInfo.iFileStatus != -1)
	{
		InitDataVessel();
	}

	//申请驱动
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

	//获取创建选项
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

	//创建文件
	pDataset = pDriver->Create(strFileName,0,0,0,GDT_Unknown,papszOptions);
	if(pDataset == NULL){strErrMsgs.push_back("pField == NULL @ CreateFile");	return FALSE;}

	//创建图层
	for(int i=0; i<VectorInfo.iLayerNum; i++)
	{
		//图层名
		CString strLayerName = "";
		if(string::size_type(i) >= VectorInfo.strLayerName.size()){strLayerName.Format("Layer %d",i);}
		else
		{
			if(VectorInfo.strLayerName[i] == ""){strLayerName.Format("Layer %d",i);}
			else{strLayerName = VectorInfo.strLayerName[i];}
		}

		//空间参考
		OGRSpatialReference SpatialRef;		SpatialRef.Clear();
		if(string::size_type(i) < VectorInfo.SpatialRef.size())
		{
			SpatialRef = VectorInfo.SpatialRef[i];
		}
		else
		{
			if(strFileExt == "TAB" || strFileExt == "MIF")
			{
				//目前默认NonEarth坐标系统
				CString str = "CoordSys NonEarth Units \"m\"";
				SpatialRef.importFromMICoordSys(str);
			}
		}

		//创建图层
		OGRLayer * pLayer = NULL;
		pLayer = pDataset->CreateLayer(strLayerName,&SpatialRef,VectorInfo.GeometryType[i],papszOptions);
		if(pLayer == NULL){strErrMsgs.push_back("pField == NULL @ CreateFile");	return FALSE;}

		//创建属性表
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


	//更新基本信息
	//已更新BasicInfo.iFileType = -1;
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

	//安全检查：如果文件未打开, 则直接退出
	if(BasicInfo.iFileStatus < 0)
	{
		CString str;	str.Format("iFileStatus = %d @ ReadSpatialInfo",BasicInfo.iFileStatus);
		strErrMsgs.push_back(str);
		return FALSE;
	}

	//安全检查：如果指定iLayerNo大于总图层数或不合法, 则直接退出
	if(iLayerNo >= BasicInfo.iLayerNum || iLayerNo < 0)
	{
		CString str;	str.Format("iLayerNo = %d @ ReadSpatialInfo",iLayerNo);
		strErrMsgs.push_back(str);
		return FALSE;
	}

	//安全检查：如果指定i64FeatureNo大于总对象数或不合法, 则直接退出
	//Mapinfo的空间对象下标从1开始,为了统一处理,在此特殊处理一下
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

	//获取操作图层句柄
	OGRLayer * pLayer = NULL;
	pLayer = pDataset->GetLayer(iLayerNo);
	if(pLayer == NULL)
	{
		strErrMsgs.push_back("pLayer = NULL @ ReadSpatialInfo");
		return FALSE;
	}

	//pLayer->ResetReading();


	//获取操作空间对象句柄
	OGRFeature * pFeature = NULL;

	/*pFeature = pLayer->GetNextFeature();
	INT64 iTmp = pFeature->GetFID();*/


	pFeature = pLayer->GetFeature(i64FeatureNo);
	if(pFeature == NULL)
	{
		strErrMsgs.push_back("pFeature = NULL @ ReadSpatialInfo");
		return FALSE;
	}


	//获取属性信息
	if(bReadAttributeInfo)
	{
		//清空数据容器
		GeoObjAttribute.uiFieldNum = 0;
		GeoObjAttribute.FieldInfoString.clear();

		//解析并保存至属性表数据容器
		GeoObjAttribute.uiFieldNum = BasicInfo.Attribute[iLayerNo].iFieldNum;	

		for(int i=0; i<BasicInfo.Attribute[iLayerNo].iFieldNum; i++)
		{
			GeoObjAttribute.FieldInfoString.push_back(ConvertTextInCoding(pFeature->GetFieldAsString(i),iCoding));
		}
	}

	//获取空间信息
	if(bReadSpatialInfo)
	{
		//清空数据容器
		GeoObjSpatial.MainGeometry.GeometryType = UNKNOWN_TYPE;
		GeoObjSpatial.MainGeometry.uiNodePointNum = 0;
		GeoObjSpatial.MainGeometry.NodePointList.clear();
		GeoObjSpatial.MainGeometry.uiRingNum = 0;
		GeoObjSpatial.MainGeometry.Rings.clear();
		GeoObjSpatial.MainGeometry.uiSubGeometryNum = 0;
		GeoObjSpatial.SubGeometrys.clear();
		GeoObjSpatial.PartGeometrys.clear();

		//获取空间对象的空间信息句柄
		OGRGeometry * pGeometry = NULL;	
		pGeometry = pFeature->GetGeometryRef();
		if(pGeometry == NULL)
		{
			strErrMsgs.push_back("pGeometry = NULL @ ReadSpatialInfo");	
			OGRFeature::DestroyFeature(pFeature);
			return FALSE;
		}

		//将空间对象的空间信息赋值到标准容器	
		OGRwkbGeometryType type = wkbFlatten(pGeometry->getGeometryType());
		GeoObjSpatial.MainGeometry.GeometryType = (GEOMETRY_TYPE)wkbFlatten(pGeometry->getGeometryType());

		//分类型进行处理,但是最后都能统一到GEO_OBJECT_SPATIAL结构体中,这样对用户来说始终操作的就是一种类型的容器,保持了接口的整洁性和易用性
		NODE_POINT PT;	RING Ring;
		OGRLinearRing * pRing = NULL;
		OGRLinearRing * pIslandRing = NULL;

		//单个点	
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

		//单个线
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

		//单个多边形
		else if(GeoObjSpatial.MainGeometry.GeometryType == SINGLE_POLYGON)
		{
			OGRPolygon * pOGRPolygon = (OGRPolygon *)pGeometry;

			//获取外多边形
			pRing = pOGRPolygon->getExteriorRing();
			GeoObjSpatial.MainGeometry.uiNodePointNum = pRing->getNumPoints();
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiNodePointNum; i++)
			{
				PT.dX = pRing->getX(i);	PT.dY = pRing->getY(i);
				GeoObjSpatial.MainGeometry.NodePointList.push_back(PT);
			}

			//获取内多边形
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

		//组合点
		else if(GeoObjSpatial.MainGeometry.GeometryType == MULTI_POINTS)
		{
			OGRMultiPoint * pMultiPoint = (OGRMultiPoint *)pGeometry;

			//得到子图元对象数目
			GeoObjSpatial.MainGeometry.uiSubGeometryNum = pMultiPoint->getNumGeometries();

			//按照单个点类型循环获取子图元对象空间信息
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

		//组合线
		else if(GeoObjSpatial.MainGeometry.GeometryType == MULTI_POLYLINES)
		{
			OGRMultiLineString * pMultiLines = (OGRMultiLineString *)pGeometry;

			//得到子图元对象数目
			GeoObjSpatial.MainGeometry.uiSubGeometryNum = pMultiLines->getNumGeometries();

			//按照单个线类型循环获取每个子图元对象的空间信息
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

		//组合多边形
		else if(GeoObjSpatial.MainGeometry.GeometryType == MULTI_POLYGONS)
		{
			OGRMultiPolygon * pMultiPolygon = (OGRMultiPolygon *)pGeometry;

			//得到子图元对象数量
			GeoObjSpatial.MainGeometry.uiSubGeometryNum = pMultiPolygon->getNumGeometries();

			//按照单个多边形类型循环获取每个子图元对象的空间信息
			BASIC_GEOMETRY SubGeometryTmp;	OGRPolygon * pOGRPolygonTmp = NULL;	OGRLinearRing * pRing = NULL;	OGRLinearRing * pIslandRing = NULL;

			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				pOGRPolygonTmp = (OGRPolygon *)pMultiPolygon->getGeometryRef(i);

				SubGeometryTmp.GeometryType = SINGLE_POLYGON;

				//获取外多边形
				pRing = pOGRPolygonTmp->getExteriorRing();
				SubGeometryTmp.uiNodePointNum = pRing->getNumPoints();
				SubGeometryTmp.NodePointList.clear();
				for(unsigned int k=0; k<SubGeometryTmp.uiNodePointNum; k++)
				{
					PT.dX = pRing->getX(i);	PT.dY = pRing->getY(i);
					SubGeometryTmp.NodePointList.push_back(PT);
				}

				//获取内多边形
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

		//组合图元
		else if(GeoObjSpatial.MainGeometry.GeometryType == MULTI_GEOMETRYS)
		{
			//第二层空间对象句柄 （第三层空间对象句柄需要根据类型临时生成）
			OGRGeometry * pGeometryTmp = NULL;		

			//第二层空间对象临时容器
			BASIC_GEOMETRY SubGeometry;

			//第三层空间对象临时容器
			BASIC_GEOMETRY PartGeometry;

			//获取第一层(MainGeometry)
			OGRGeometryCollection * pCollection = (OGRGeometryCollection *)pGeometry;
			GeoObjSpatial.MainGeometry.uiSubGeometryNum = pCollection->getNumGeometries();			

			//循环获取第二层(SubGeometry)
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				//构造第二层空间对象
				pGeometryTmp = (OGRGeometry *)pCollection->getGeometryRef(i);
				SubGeometry.GeometryType = (GEOMETRY_TYPE)wkbFlatten(pGeometryTmp->getGeometryType());
				SubGeometry.uiNodePointNum = 0;	SubGeometry.NodePointList.clear();
				SubGeometry.uiRingNum = 0;		SubGeometry.Rings.clear();
				//注意第二层的子对象数量需要在具体获取第三层数据时获取,不要忘了！

				//单个点
				if(SubGeometry.GeometryType == SINGLE_POINT)
				{
					OGRPoint * pPoint = (OGRPoint *)pGeometryTmp;

					//更新第二层空间对象的子对象数目
					SubGeometry.uiSubGeometryNum = 1;

					//获取第三层的单个点数据
					{
						//类型
						PartGeometry.GeometryType = SINGLE_POINT;

						//外环
						PartGeometry.uiNodePointNum = 1;
						PartGeometry.NodePointList.clear();
						PT.dX = pPoint->getX();
						PT.dY = pPoint->getY();				
						PartGeometry.NodePointList.push_back(PT);

						//内环
						PartGeometry.uiRingNum = 0;
						PartGeometry.Rings.clear();

						//子对象数
						PartGeometry.uiSubGeometryNum = 0;
					}
					GeoObjSpatial.PartGeometrys.push_back(PartGeometry);				
				}

				//单个线
				else if(SubGeometry.GeometryType == SINGLE_POLYLINE)
				{
					OGRLineString * pOGRLine = (OGRLineString *)pGeometryTmp;

					//更新第二层空间对象的子对象数目
					SubGeometry.uiSubGeometryNum = 1;

					//获取第三层的单个线数据
					{
						//类型
						PartGeometry.GeometryType = SINGLE_POLYLINE;

						//外环
						PartGeometry.uiNodePointNum = pOGRLine->getNumPoints();
						PartGeometry.NodePointList.clear();
						for(unsigned int n=0; n<PartGeometry.uiNodePointNum; n++)
						{
							PT.dX = pOGRLine->getX(n);	PT.dY = pOGRLine->getY(n);
							PartGeometry.NodePointList.push_back(PT);
						}

						//内环
						PartGeometry.uiRingNum = 0;
						PartGeometry.Rings.clear();

						//子对象数
						PartGeometry.uiSubGeometryNum = 0;
					}
					GeoObjSpatial.PartGeometrys.push_back(PartGeometry);
				}

				//单个多边形
				else if(SubGeometry.GeometryType == SINGLE_POLYGON)
				{
					OGRPolygon * pOGRPolygon = (OGRPolygon *)pGeometryTmp;

					//更新第二层空间对象的子对象数目
					SubGeometry.uiSubGeometryNum = 1;

					//获取第三层的单个多边形数据
					{
						//类型
						PartGeometry.GeometryType = SINGLE_POLYGON;

						//外环
						OGRLinearRing * pRing = pOGRPolygon->getExteriorRing();
						PartGeometry.uiNodePointNum = pRing->getNumPoints();
						PartGeometry.NodePointList.clear();
						for(unsigned int n=0; n<PartGeometry.uiNodePointNum; n++)
						{
							PT.dX = pRing->getX(n);	PT.dY = pRing->getY(n);
							PartGeometry.NodePointList.push_back(PT);
						}

						//内环
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

						//子对象数
						PartGeometry.uiSubGeometryNum = 0;
					}
					GeoObjSpatial.PartGeometrys.push_back(PartGeometry);
				}

				//组合点
				else if(SubGeometry.GeometryType == MULTI_POINTS)
				{
					OGRMultiPoint * pMultiPoint = (OGRMultiPoint *)pGeometryTmp;

					//更新第二层空间对象的子对象数目
					SubGeometry.uiSubGeometryNum = pMultiPoint->getNumGeometries();

					//获取第三层的多个点数据
					for(unsigned int k=0; k<SubGeometry.uiSubGeometryNum; k++)
					{
						OGRPoint * pOGRPointTmp = (OGRPoint *)pMultiPoint->getGeometryRef(k);

						//类型
						PartGeometry.GeometryType = SINGLE_POINT;

						//外环
						PartGeometry.uiNodePointNum = 1;
						PartGeometry.NodePointList.clear();
						PT.dX = pOGRPointTmp->getX();
						PT.dY = pOGRPointTmp->getY();
						PartGeometry.NodePointList.push_back(PT);

						//内环
						PartGeometry.uiRingNum = 0;
						PartGeometry.Rings.clear();

						//子对象树
						PartGeometry.uiSubGeometryNum = 0;

						GeoObjSpatial.PartGeometrys.push_back(PartGeometry);
					}
				}

				//组合线
				else if(SubGeometry.GeometryType == MULTI_POLYLINES)
				{
					OGRMultiLineString * pMultiLines = (OGRMultiLineString *)pGeometryTmp;

					//更新第二层空间对象的子对象数目
					SubGeometry.uiSubGeometryNum = pMultiLines->getNumGeometries();

					//获取第三层的多个线数据
					for(unsigned int k=0; k<SubGeometry.uiSubGeometryNum; k++)
					{
						OGRLineString * pOGRLineTmp = (OGRLineString *)pMultiLines->getGeometryRef(k);

						//类型
						PartGeometry.GeometryType = SINGLE_POLYLINE;

						//外环
						PartGeometry.uiNodePointNum = pOGRLineTmp->getNumPoints();
						PartGeometry.NodePointList.clear();
						for(unsigned int n=0; n<PartGeometry.uiNodePointNum; n++)
						{
							PT.dX = pOGRLineTmp->getX(n);
							PT.dY = pOGRLineTmp->getY(n);
							PartGeometry.NodePointList.push_back(PT);
						}

						//内环
						PartGeometry.uiRingNum = 0;
						PartGeometry.Rings.clear();

						//子对象数
						PartGeometry.uiSubGeometryNum = 0;

						GeoObjSpatial.PartGeometrys.push_back(PartGeometry);
					}	
				}

				//组合多边形
				else if(SubGeometry.GeometryType == MULTI_POLYGONS)
				{
					OGRMultiPolygon * pMultiPolygon = (OGRMultiPolygon *)pGeometryTmp;

					//更新第二层空间对象的子对象数目
					SubGeometry.uiSubGeometryNum = pMultiPolygon->getNumGeometries();

					//获取第三层的多个多边形数据
					for(unsigned int k=0; k<SubGeometry.uiSubGeometryNum; k++)
					{
						OGRPolygon * pOGRPolygonTmp = (OGRPolygon *)pMultiPolygon->getGeometryRef(k);

						//类型
						PartGeometry.GeometryType = SINGLE_POLYGON;

						//外环
						pRing = pOGRPolygonTmp->getExteriorRing();
						PartGeometry.uiNodePointNum = pRing->getNumPoints();
						PartGeometry.NodePointList.clear();
						for(unsigned int n=0; n<PartGeometry.uiNodePointNum; n++)
						{
							PT.dX = pRing->getX(n);	PT.dY = pRing->getY(n);
							PartGeometry.NodePointList.push_back(PT);
						}

						//内环
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

						//子对象数
						PartGeometry.uiSubGeometryNum = 0;

						GeoObjSpatial.PartGeometrys.push_back(PartGeometry);
					}

				}

				//其它未知类型错误
				else
				{
					CString str;str.Format("GEOMETRY_TYPE Error: %d @ ReadSpatialInfo - MULTI_GEOMETRYS",SubGeometry.GeometryType);
					strErrMsgs.push_back(str);
					return FALSE;
				}

				//更新第二层空间对象数据
				GeoObjSpatial.SubGeometrys.push_back(SubGeometry);
			}

			OGRFeature::DestroyFeature(pFeature);

			return TRUE;
		}

		//其他未知类型错误
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

	//安全检查: 如果文件未打开, 则直接退出
	if(BasicInfo.iFileStatus < 0)
	{
		CString str;	str.Format("iFileStatus = %d @ ReadAttributeInfo",BasicInfo.iFileStatus);
		strErrMsgs.push_back(str);
		return FALSE;
	}

	//获取图层句柄
	OGRLayer * pLayer = NULL;
	pLayer = pDataset->GetLayer(iLayerNo);
	if(pLayer == NULL)
	{
		strErrMsgs.push_back("pLyaer == NULL @ WriteSpatialInfo");
		return FALSE;
	}

	//获取操作对象句柄
	OGRFeature * pFeature = NULL;

	//新建对象
	if(bCreateNewFeature == TRUE)
	{
		pFeature = OGRFeature::CreateFeature(pLayer->GetLayerDefn());
	}

	//更新已有对象
	else
	{
		//安全检查: 判断i64FeatureNo是否合法
		if(i64FeatureNo < 0 || i64FeatureNo > BasicInfo.i64FeatureNum[iLayerNo])
		{
			CString str;
			str.Format("i64FeatureNo[%d] / TotalFeatureNum[%d] Error! @ WriteGeoObject",i64FeatureNo,BasicInfo.i64FeatureNum[iLayerNo]);
			strErrMsgs.push_back(str);
			return FALSE;
		}

		//安全检查: 判断当前图层是否支持随机读写（即指定对象编号进行更新）
		if(TRUE != pLayer->TestCapability(OLCRandomWrite))
		{
			strErrMsgs.push_back("pLayer is not support RandomWrite @ WriteGeoObject");
			return FALSE;
		}

		//特殊处理: 如果为MapInfo文件,空间对象下标从1开始,其它从0开始
		if(BasicInfo.enFileType == MAPINFO_MIF_FILE || BasicInfo.enFileType == MAPINFO_TAB_FILE)
		{
			i64FeatureNo++;
		}

		//获取当前操作空间对象句柄
		pFeature = pLayer->GetFeature(i64FeatureNo);
	}

	if(pFeature == NULL)
	{
		strErrMsgs.push_back("pFeature = NULL @ WriteGeoObject");
		return FALSE;
	}

	//更新空间对象的属性信息
	if(bUpdateAttributeInfo == TRUE)
	{
		//安全检查
		if(int(GeoObjAttribute.uiFieldNum) < BasicInfo.Attribute[iLayerNo].iFieldNum || int(GeoObjAttribute.FieldInfoString.size()) < BasicInfo.Attribute[iLayerNo].iFieldNum)
		{
			CString str;
			str.Format("GeoObjAttribute.uiFieldNum[%d] is less than BasicInfo.Attribute[%d].iFieldNum[%d]",GeoObjAttribute.uiFieldNum,iLayerNo,BasicInfo.Attribute[iLayerNo].iFieldNum);
			strErrMsgs.push_back(str);
			return FALSE;
		}

		//添加空间对象的属性信息
		for(int i=0; i<BasicInfo.Attribute[iLayerNo].iFieldNum; i++)
		{
			pFeature->SetField(BasicInfo.Attribute[iLayerNo].strFieldName[i],GeoObjAttribute.FieldInfoString[i]);
		}
	}

	//更新对象的空间信息
	if(bUpdateSpatialInfo == TRUE)
	{
		//根据不同的图元类型添加空间对象的空间信息
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
			//单个点
		case SINGLE_POINT:
			//安全检查
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

			//获取单个点信息
			PointTmp.empty();
			PointTmp.setX(GeoObjSpatial.MainGeometry.NodePointList[0].dX);
			PointTmp.setY(GeoObjSpatial.MainGeometry.NodePointList[0].dY);

			//设置空间对象
			if(pFeature->SetGeometry(&PointTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - SINGLE_POINT");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}			
			break;

			//单个线
		case SINGLE_POLYLINE:
			//安全检查
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

			//获取单个线信息
			LineTmp.empty();
			for(unsigned int n=0; n<GeoObjSpatial.MainGeometry.uiNodePointNum; n++)
			{
				LineTmp.addPoint(GeoObjSpatial.MainGeometry.NodePointList[n].dX,GeoObjSpatial.MainGeometry.NodePointList[n].dY);
			}

			//设置空间对象
			if(pFeature->SetGeometry(&LineTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - SINGLE_POLYLINE");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}		
			break;


			//单个多边形
		case SINGLE_POLYGON:
			//安全检查
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

			//添加空间信息
			PolygonTmp.empty();	RingTmp.empty();
			//外环
			for(unsigned int n=0; n<GeoObjSpatial.MainGeometry.uiNodePointNum; n++)
			{
				RingTmp.addPoint(GeoObjSpatial.MainGeometry.NodePointList[n].dX, GeoObjSpatial.MainGeometry.NodePointList[n].dY);
			}
			RingTmp.closeRings();
			PolygonTmp.addRing(&RingTmp);

			//内环
			for(unsigned int j=0; j<GeoObjSpatial.MainGeometry.uiRingNum; j++)
			{
				RingTmp.empty();
				//安全检查
				if(GeoObjSpatial.MainGeometry.Rings[j].uiNodePointNum < 3 || GeoObjSpatial.MainGeometry.Rings[j].NodePointList.size() < 3)
				{
					CString str;
					str.Format("GeoObjSpatial.Ring[%d].uiNodePointNum is less than 3! @ WriteGeoObject - SINGLE_POLYGON",j);
					strErrMsgs.push_back(str);
					OGRFeature::DestroyFeature(pFeature);
					return FALSE;
				}
				//添加空间信息
				for(unsigned int n=0; n<GeoObjSpatial.MainGeometry.Rings[j].uiNodePointNum; n++)
				{
					RingTmp.addPoint(GeoObjSpatial.MainGeometry.Rings[j].NodePointList[n].dX, GeoObjSpatial.MainGeometry.Rings[j].NodePointList[n].dY);
				}
				RingTmp.closeRings();
				PolygonTmp.addRing(&RingTmp);
			}

			//设置空间对象
			if(pFeature->SetGeometry(&PolygonTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - SINGLE_POLYGON");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			break;

			//组合点
		case MULTI_POINTS:
			//安全检查
			if(BasicInfo.enFileType == ESRI_SHAPE_FILE && BasicInfo.GeometryType[iLayerNo] != wkbPoint)
			{
				strErrMsgs.push_back("FILE TYPE & GEOMETRY TYPE OF LAYER IS NOT MATCH! @ WriteGeoObject - MULTI_POINTS");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}

			//添加空间信息
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				//安全检查
				if(GeoObjSpatial.SubGeometrys[i].uiNodePointNum < 1 || GeoObjSpatial.SubGeometrys[i].NodePointList.size() < 1)
				{
					CString str;
					str.Format("GeoObjSpatial.SubGeometrys[%d].uiNodePointNum is less than 1 @ WriteGeoObject - MULTI_POINTS",i);
					strErrMsgs.push_back(str);
					OGRFeature::DestroyFeature(pFeature);
					return FALSE;
				}

				//添加空间信息
				PointTmp.empty();
				PointTmp.setX(GeoObjSpatial.SubGeometrys[i].NodePointList[0].dX);
				PointTmp.setY(GeoObjSpatial.SubGeometrys[i].NodePointList[0].dY);

				MultiPointTmp.addGeometry(&PointTmp);
			}

			//设置空间对象
			if(pFeature->SetGeometry(&MultiPointTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - MULTI_POINTS");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			break;

			//组合线
		case MULTI_POLYLINES:
			//安全检查
			if(BasicInfo.enFileType == ESRI_SHAPE_FILE && BasicInfo.GeometryType[iLayerNo] != wkbLineString)
			{
				strErrMsgs.push_back("FILE TYPE & GEOMETRY TYPE OF LAYER IS NOT MATCH! @ WriteGeoObject - MULTI_POLYLINES");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}

			//添加空间信息
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				//安全检查
				if(GeoObjSpatial.SubGeometrys[i].uiNodePointNum < 2 || GeoObjSpatial.SubGeometrys[i].NodePointList.size() < 2)
				{
					CString str;
					str.Format("GeoObjSpatial.SubGeometrys[%d].uiNodePointNum is less than 2! @ WriteGeoObject - MULTI_POLYLINES",i);
					strErrMsgs.push_back(str);
					OGRFeature::DestroyFeature(pFeature);
					return FALSE;
				}

				//添加空间信息
				LineTmp.empty();
				for(unsigned int n=0; n<GeoObjSpatial.SubGeometrys[i].uiNodePointNum; n++)
				{
					LineTmp.addPoint(GeoObjSpatial.SubGeometrys[i].NodePointList[n].dX, GeoObjSpatial.SubGeometrys[i].NodePointList[n].dY);
				}

				MultiLineTmp.addGeometry(&LineTmp);
			}

			//设置空间对象
			if(pFeature->SetGeometry(&MultiLineTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - MULTI_POLYLINES");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			break;

			//组合多边形
		case MULTI_POLYGONS:
			//安全检查
			if(BasicInfo.enFileType == ESRI_SHAPE_FILE && BasicInfo.GeometryType[iLayerNo] != wkbPolygon)
			{
				strErrMsgs.push_back("FILE TYPE & GEOMETRY TYPE OF LAYER IS NOT MATCH! @ WriteGeoObject - MULTI_POLYGONS");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}

			//添加空间信息
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				PolygonTmp.empty();

				//安全检查
				if(GeoObjSpatial.SubGeometrys[i].uiNodePointNum < 3 || GeoObjSpatial.SubGeometrys[i].NodePointList.size() < 3)
				{
					CString str;
					str.Format("GeoObjSpatial.SubGeometrys[%d].uiNodePointNum is less than 3! @ WriteGeoObject - MULTI_POLYGONS",i);
					strErrMsgs.push_back(str);
					OGRFeature::DestroyFeature(pFeature);
					return FALSE;
				}

				//添加空间信息					
				//外环
				RingTmp.empty();
				for(unsigned int n=0; n<GeoObjSpatial.SubGeometrys[i].uiNodePointNum; n++)
				{
					RingTmp.addPoint(GeoObjSpatial.SubGeometrys[i].NodePointList[n].dX, GeoObjSpatial.SubGeometrys[i].NodePointList[n].dY);
				}
				RingTmp.closeRings();
				PolygonTmp.addRing(&RingTmp);

				//内环
				for(unsigned int j=0; j<GeoObjSpatial.SubGeometrys[i].uiRingNum; j++)
				{
					//安全检查
					if(GeoObjSpatial.SubGeometrys[i].Rings[j].uiNodePointNum < 3 || GeoObjSpatial.SubGeometrys[i].Rings[j].NodePointList.size() < 3)
					{
						CString str;
						str.Format("GeoObjSpatial.SubGeometrys[%d].Rings[%d].uiNodePointNum is less than 3! @ WriteGeoObject - MULTI_POLYGONS",i,j);
						strErrMsgs.push_back(str);
						OGRFeature::DestroyFeature(pFeature);
						return FALSE;
					}
					//添加空间信息
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

			//设置空间对象
			if(pFeature->SetGeometry(&MultiPolygonTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - MULTI_POLYGONS");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			break;

			//组合图元
		case MULTI_GEOMETRYS:
			//安全检查
			if(BasicInfo.enFileType == ESRI_SHAPE_FILE)
			{
				strErrMsgs.push_back("FILE TYPE IS NOT SUPPORT THIS GEOMETRY TYPE! @ WriteGeoObject - MULTI_GEOMETRYS");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}

			//添加空间信息
			uiPartGeometryPointer = 0;
			for(unsigned int i=0; i<GeoObjSpatial.MainGeometry.uiSubGeometryNum; i++)
			{
				//单个点
				if(GeoObjSpatial.SubGeometrys[i].GeometryType == SINGLE_POINT)
				{
					//安全检查
					if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].uiNodePointNum < 1 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList.size() < 1)
					{
						CString str;
						str.Format("GeoObjSpatial.PartGeometrys[%d].uiNodePointNum is less than 1 @ WriteGeoObject - MULTI_GEOMTRYS - SINGLE_POINT",uiPartGeometryPointer);
						strErrMsgs.push_back(str);
						OGRFeature::DestroyFeature(pFeature);
						return FALSE;
					}

					//外环
					PointTmp.empty();
					PointTmp.setX(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList[0].dX);
					PointTmp.setY(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList[0].dY);
					MultiCollectionTmp.addGeometry(&PointTmp);

					//移动第三层空间对象容器指针
					uiPartGeometryPointer++;
				}

				//单个线
				else if(GeoObjSpatial.SubGeometrys[i].GeometryType == SINGLE_POLYLINE)
				{
					//安全检查
					if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].uiNodePointNum < 2 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList.size() < 2)
					{
						CString str;
						str.Format("GeoObjSpatial.PartGeometrys[%d].uiNodePointNum is less than 2 @ WriteGeoObject - MULTI_GEOMTRYS - SINGLE_POLYLINE",uiPartGeometryPointer);
						strErrMsgs.push_back(str);
						OGRFeature::DestroyFeature(pFeature);
						return FALSE;
					}

					//外环
					LineTmp.empty();
					for(unsigned int n=0; n<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].uiNodePointNum; n++)
					{
						LineTmp.addPoint(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList[n].dX, GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList[n].dY);
					}
					MultiCollectionTmp.addGeometry(&LineTmp);

					//移动第三层空间对象容器指针
					uiPartGeometryPointer++;
				}

				//单个多边形
				else if(GeoObjSpatial.SubGeometrys[i].GeometryType == SINGLE_POLYGON)
				{
					//安全检查
					if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].uiNodePointNum < 3 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList.size() < 3)
					{
						CString str;
						str.Format("GeoObjSpatial.PartGeometrys[%d].uiNodePointNum is less than 3 @ WriteGeoObject - MULTI_GEOMTRYS - SINGLE_POLYGON",uiPartGeometryPointer);
						strErrMsgs.push_back(str);
						OGRFeature::DestroyFeature(pFeature);
						return FALSE;
					}

					//外环
					PolygonTmp.empty();	RingTmp.empty();
					for(unsigned int n=0; n<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].uiNodePointNum; n++)
					{
						RingTmp.addPoint(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList[n].dX, GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].NodePointList[n].dY);
					}
					RingTmp.closeRings();
					PolygonTmp.addRing(&RingTmp);

					//内环
					for(unsigned int j=0; j<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer].uiRingNum; j++)
					{
						//安全检查
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

					//移动第三层空间对象容器指针
					uiPartGeometryPointer++;
				}

				//多个点
				else if(GeoObjSpatial.SubGeometrys[i].GeometryType == MULTI_POINTS)
				{
					MultiPointTmp.empty();

					for(unsigned int k=0; k<GeoObjSpatial.SubGeometrys[i].uiSubGeometryNum; k++)
					{
						//安全检查
						if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].uiNodePointNum < 1 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList.size() < 1)
						{
							CString str;
							str.Format("GeoObjSpatial.PartGeometrys[%d].uiNodePointNum is less than 1 @ WriteGeoObject - MULTI_GEOMTRYS - MULTI_POINTS",uiPartGeometryPointer + k);
							strErrMsgs.push_back(str);
							OGRFeature::DestroyFeature(pFeature);
							return FALSE;
						}

						//外环
						PointTmp.empty();
						PointTmp.setX(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList[0].dX);
						PointTmp.setY(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList[0].dY);
						MultiPointTmp.addGeometry(&PointTmp);
					}

					MultiCollectionTmp.addGeometry(&MultiPointTmp);

					//移动第三层空间对象容器指针
					uiPartGeometryPointer += GeoObjSpatial.SubGeometrys[i].uiSubGeometryNum;
				}

				//多个线
				else if(GeoObjSpatial.SubGeometrys[i].GeometryType == MULTI_POLYLINES)
				{
					MultiLineTmp.empty();

					for(unsigned int k=0; k<GeoObjSpatial.SubGeometrys[i].uiSubGeometryNum; k++)
					{
						//安全检查
						if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].uiNodePointNum < 2 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList.size() < 2)
						{
							CString str;
							str.Format("GeoObjSpatial.PartGeometrys[%d].uiNodePointNum is less than 2 @ WriteGeoObject - MULTI_GEOMTRYS - MULTI_POLYLINES",uiPartGeometryPointer + k);
							strErrMsgs.push_back(str);
							OGRFeature::DestroyFeature(pFeature);
							return FALSE;
						}

						//外环
						LineTmp.empty();

						for(unsigned int n=0; n<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].uiNodePointNum; n++)
						{
							LineTmp.addPoint(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList[n].dX, GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList[n].dY);
						}

						MultiLineTmp.addGeometry(&LineTmp);
					}

					MultiCollectionTmp.addGeometry(&MultiLineTmp);

					//移动第三层空间对象容器指针
					uiPartGeometryPointer += GeoObjSpatial.SubGeometrys[i].uiSubGeometryNum;
				}

				//多个多边形
				else if(GeoObjSpatial.SubGeometrys[i].GeometryType == MULTI_POLYGONS)
				{

					MultiPolygonTmp.empty();

					for(unsigned int k=0; k<GeoObjSpatial.SubGeometrys[i].uiSubGeometryNum; k++)
					{
						//安全检查
						if(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].uiNodePointNum < 3 || GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList.size() < 3)
						{
							CString str;
							str.Format("GeoObjSpatial.PartGeometrys[%d].uiNodePointNum is less than 3 @ WriteGeoObject - MULTI_GEOMTRYS - MULTI_POLYGONS",uiPartGeometryPointer + k);
							strErrMsgs.push_back(str);
							OGRFeature::DestroyFeature(pFeature);
							return FALSE;
						}

						//外环
						PolygonTmp.empty();		RingTmp.empty();
						for(unsigned int n=0; n<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].uiNodePointNum; n++)
						{
							RingTmp.addPoint(GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList[n].dX, GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].NodePointList[n].dY);
						}
						RingTmp.closeRings();
						PolygonTmp.addRing(&RingTmp);

						//内环
						for(unsigned int j=0; j<GeoObjSpatial.PartGeometrys[uiPartGeometryPointer + k].uiRingNum; j++)
						{
							//安全检查
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

					//移动第三层空间对象容器指针
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

			//设置空间对象
			if(pFeature->SetGeometry(&MultiCollectionTmp) != OGRERR_NONE)
			{
				strErrMsgs.push_back("SetGeometry Error @ WriteGeoObject - MULTI_GEOMETRYS");
				OGRFeature::DestroyFeature(pFeature);
				return FALSE;
			}
			break;

			//未知类型错误
		default:
			CString str;
			str.Format("GeoObjSpatial.MainGeometry.GeometryType Error: %d, @  WriteGeoObject",GeoObjSpatial.MainGeometry.GeometryType);
			strErrMsgs.push_back(str);
			OGRFeature::DestroyFeature(pFeature);
			return FALSE;
		}

		//释放容器
		PointTmp.empty();		LineTmp.empty();		RingTmp.empty(); 			PolygonTmp.empty();	
		MultiPointTmp.empty();	MultiLineTmp.empty();	MultiPolygonTmp.empty();	MultiCollectionTmp.empty();
	}

	//在当前操作图层中新建或更新空间对象
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

	//安全检查：如果文件未打开, 则直接退出
	if(BasicInfo.iFileStatus < 0)
	{
		CString str;	str.Format("iFileStatus = %d @ DeleteGeoObject",BasicInfo.iFileStatus);
		strErrMsgs.push_back(str);
		return FALSE;
	}

	//安全检查：如果指定iLayerNo大于总图层数或不合法, 则直接退出
	if(iLayerNo >= BasicInfo.iLayerNum || iLayerNo < 0)
	{
		CString str;	str.Format("iLayerNo = %d @ DeleteGeoObject",iLayerNo);
		strErrMsgs.push_back(str);
		return FALSE;
	}

	//安全检查：如果指定i64FeatureNo大于总对象数或不合法, 则直接退出
	//Mapinfo的空间对象下标从1开始,为了统一处理,在此特殊处理一下
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

	//获取操作图层句柄
	OGRLayer * pLayer = NULL;
	pLayer = pDataset->GetLayer(iLayerNo);
	if(pLayer == NULL)
	{
		strErrMsgs.push_back("pLayer = NULL @ DeleteGeoObject");
		return FALSE;
	}

	//安全检查:判断当前操作图层是否支持删除空间对象
	if(TRUE != pLayer->TestCapability(OLCDeleteFeature))
	{
		strErrMsgs.push_back("pLayer is not support RandomWrite @ WriteGeoObject");
		return FALSE;
	}

	//删除空间对象（DeleteFeature仅能标记该对象需要被删除）
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
		//DeleteFeature只是在数据集中做了标记，真正删除需要在CloseFile时进行PACK操作！
		bDeleteFeature = TRUE;

		return TRUE;
	}
}
// GDALTestDlg.h : 头文件
#include "RasterIO1.h"

#pragma once


// CGDALTestDlg 对话框
class CGDALTestDlg : public CDialog
{
// 构造
public:
	CGDALTestDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_GDALTEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBnReadImage();
	afx_msg void OnBnClickedBnReadVector();
};

// This file is part of UltraVNC
// https://github.com/ultravnc/UltraVNC
// https://uvnc.com/
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// SPDX-FileCopyrightText: Copyright (C) 2002-2025 UltraVNC Team Members. All Rights Reserved.
// SPDX-FileCopyrightText: Copyright (C) 1999-2002 Vdacc-VNC & eSVNC Projects. All Rights Reserved.
//


// Clipboard.h

// adzm - July 2010
//
// Common classes for dealing with the clipboard, including serializing and deserializing compressed data, hashing and comparing, etc.
// Used by server and viewer.

#include "Clipboard.h"


#define VC_EXTRALEAN
#include <winsock2.h>
#include <windows.h>

#include <string>
#include <vector>
#include <cstdint>

#include <rdr/MemInStream.h>
#include <rdr/ZlibOutStream.h>
#include <rdr/ZlibInStream.h>
#ifdef _VCPKG
#include  <zlib.h>
#include  <zstd.h>
#else
#include "../zlib/zlib.h"
#include "../zstd/lib/zstd.h"
#endif





ExtendedClipboardDataMessage::ExtendedClipboardDataMessage()
	: m_pExtendedData(NULL), m_nInternalLength(0), m_pCurrentPos(NULL), m_pData(NULL)
{
	EnsureBufferLength(sz_rfbExtendedClipboardData);
	m_pCurrentPos += sz_rfbExtendedClipboardData;
}

ExtendedClipboardDataMessage::~ExtendedClipboardDataMessage()
{
	delete[] m_pData;
}

void ExtendedClipboardDataMessage::Reset()
{
	delete[] m_pData;
	m_pData = NULL;
	m_pCurrentPos = NULL;
	m_pExtendedData = NULL;
	m_pCurrentPos = NULL;
	m_nInternalLength = 0;

	EnsureBufferLength(sz_rfbExtendedClipboardData);
	m_pCurrentPos += sz_rfbExtendedClipboardData;
}

void ExtendedClipboardDataMessage::AddFlag(CARD32 flag)
{
	m_pExtendedData->flags |= Swap32IfLE(flag);
}

bool ExtendedClipboardDataMessage::HasFlag(CARD32 flag)
{
	return (m_pExtendedData->flags & Swap32IfLE(flag)) ? true : false;
}

int ExtendedClipboardDataMessage::GetMessageLength()
{
	return GetDataLength() - sz_rfbExtendedClipboardData;
}

int ExtendedClipboardDataMessage::GetDataLength()
{
	return (int)(m_pCurrentPos - m_pData);
}

const BYTE* ExtendedClipboardDataMessage::GetData()
{
	return m_pData;
}

BYTE* ExtendedClipboardDataMessage::GetBuffer()
{
	return m_pData;
}

int ExtendedClipboardDataMessage::GetBufferLength()
{
	return m_nInternalLength;
}

const BYTE* ExtendedClipboardDataMessage::GetCurrentPos()
{
	return m_pCurrentPos;
}

void ExtendedClipboardDataMessage::AppendInt(CARD32 val)
{
	EnsureBufferLength(GetDataLength() + sizeof(val));

	val = Swap32IfLE(val);

	memcpy(m_pCurrentPos, &val, sizeof(val));
	m_pCurrentPos += sizeof(val);
}

void ExtendedClipboardDataMessage::AppendBytes(BYTE* pData, int length)
{
	EnsureBufferLength(GetDataLength() + length, false);

	memcpy(m_pCurrentPos, pData, length);
	m_pCurrentPos += length;
}

void ExtendedClipboardDataMessage::Advance(int len)
{
	EnsureBufferLength(GetDataLength() + len, false);
	m_pCurrentPos += len;
}

CARD32 ExtendedClipboardDataMessage::ReadInt()
{
	CARD32 val = 0;

	memcpy(&val, m_pCurrentPos, sizeof(val));

	m_pCurrentPos += sizeof(val);

	return Swap32IfLE(val);
}

void ExtendedClipboardDataMessage::EnsureBufferLength(int len, bool bGrowBeyond)
{
	if (m_nInternalLength < len) {
		int nCurrentOffset = GetDataLength();

		int nNewLength = bGrowBeyond ? len * 2 : len;
		BYTE* pNewBuffer = new BYTE[nNewLength];
		::ZeroMemory(pNewBuffer, nNewLength);

		if (m_pData) {
			memcpy(pNewBuffer, m_pData, GetDataLength());
			delete[] m_pData;
		}

		m_pData = pNewBuffer;
		m_pExtendedData = (rfbExtendedClipboardData*)m_pData;
		m_pCurrentPos = m_pData + nCurrentOffset;
		m_nInternalLength = nNewLength;
	}
}

int ExtendedClipboardDataMessage::CountFormats()
{
	CARD32 flags = GetFlags();

	flags = flags & 0x0000FFFF;

	// from Brian W. Kernighan
	int c = 0;
	for (c = 0; flags; c++) {
		flags &= flags - 1;
	}

	return c;
}

CARD32 ExtendedClipboardDataMessage::GetFlags()
{
	return Swap32IfLE(m_pExtendedData->flags);
}

const UINT ClipboardSettings::formatUnicodeText =	CF_UNICODETEXT;
const UINT ClipboardSettings::formatRTF =			RegisterClipboardFormatW(L"Rich Text Format");
const UINT ClipboardSettings::formatHTML =			RegisterClipboardFormatW(L"HTML Format");
const UINT ClipboardSettings::formatDIB =			CF_DIB;

const int ClipboardSettings::defaultLimitText =		((int)0x00A00000); // 10 megabytes uncompressed. Pretty huge, but not a problem for a LAN. Better than the previous no limit, though.
const int ClipboardSettings::defaultLimitRTF =		((int)0x00200000); // Limit these to 2 megabytes so they don't end up adding too much strain by being enabled by default
const int ClipboardSettings::defaultLimitHTML =		((int)0x00200000); // besides, every 2mb rtf/html data I have seen will DEFLATE very well.
const int ClipboardSettings::defaultLimitDIB =		((int)0x02800000); // 40 megabytes uncompressed (supports large/4K screenshots)

const int ClipboardSettings::defaultLimit =			((int)0x00200000);

ClipboardSettings::ClipboardSettings(CARD32 caps)
	: m_bSupportsEx(false)
	, m_nLimitText(defaultLimitText)
	, m_nLimitRTF(defaultLimitRTF)
	, m_nLimitHTML(defaultLimitHTML)
	, m_nLimitDIB(defaultLimitDIB)
	, m_nRequestedLimitText(m_nLimitText)
	, m_nRequestedLimitRTF(m_nLimitRTF)
	, m_nRequestedLimitHTML(m_nLimitHTML)
	, m_nRequestedLimitDIB(m_nLimitDIB)
	, m_myCaps(caps)
	, m_remoteCaps(ClipboardSettings::defaultCaps)
{
}

CARD32 ClipboardSettings::defaultCaps = 
	(clipCaps | clipRequest | clipProvide)  // capabilities
	| 
	(clipText | clipRTF | clipHTML | clipDIB); // supports Unicode text, RTF, and HTML, and DIB
CARD32 ClipboardSettings::defaultViewerCaps = defaultCaps | clipNotify;
CARD32 ClipboardSettings::defaultServerCaps = defaultCaps | clipPeek;


void ClipboardSettings::PrepareCapsPacket(ExtendedClipboardDataMessage& extendedDataMessage)
{
	// messages and formats that we can handle
	extendedDataMessage.m_pExtendedData->flags = Swap32IfLE(m_myCaps);

	// now include our limits in order of enum value
	extendedDataMessage.AppendInt(m_nLimitText);
	extendedDataMessage.AppendInt(m_nLimitRTF);
	extendedDataMessage.AppendInt(m_nLimitHTML);
	extendedDataMessage.AppendInt(m_nLimitDIB);
}

void ClipboardSettings::HandleCapsPacket(ExtendedClipboardDataMessage& extendedDataMessage, bool bSetLimits)
{
	int nCount = extendedDataMessage.CountFormats();

	m_remoteCaps = extendedDataMessage.GetFlags();	

	if (m_remoteCaps & clipText) {
		m_nRequestedLimitText = (int)extendedDataMessage.ReadInt();		
		nCount--;
	} else {
		m_nRequestedLimitText = 0;
	}
	if (m_remoteCaps & clipRTF) {
		m_nRequestedLimitRTF = (int)extendedDataMessage.ReadInt();
		nCount--;
	} else {
		m_nRequestedLimitRTF = 0;
	}
	if (m_remoteCaps & clipHTML) {
		m_nRequestedLimitHTML = (int)extendedDataMessage.ReadInt();
		nCount--;
	} else {
		m_nRequestedLimitHTML = 0;
	}
	if (m_remoteCaps & clipDIB) {
		m_nRequestedLimitDIB = (int)extendedDataMessage.ReadInt();
		nCount--;
	} else {
		m_nRequestedLimitDIB = 0;
	}

	if (bSetLimits) {
		m_nLimitText = m_nRequestedLimitText;
		m_nLimitRTF = m_nRequestedLimitRTF;
		m_nLimitHTML = m_nRequestedLimitHTML;
		m_nLimitDIB = m_nRequestedLimitDIB;
	}

	// read any unsupported values
	while (nCount) {
		extendedDataMessage.ReadInt();
		nCount--;
	}
}

ClipboardHolder::ClipboardHolder(HWND hwndOwner)
	: m_bIsOpen(false)
{
	for (int attempt = 0; attempt < 10; ++attempt) {
		if (::OpenClipboard(hwndOwner)) {
			m_bIsOpen = true;
			break;
		}
		::Sleep(15);
	}
}

ClipboardHolder::~ClipboardHolder()
{
	if (m_bIsOpen) {
		::CloseClipboard();
	}
}

ClipboardData::ClipboardData()
	: m_crc(0)
	, m_lengthText(0)
	, m_lengthRTF(0)
	, m_lengthHTML(0)
	, m_lengthDIB(0)
	, m_pDataText(NULL)
	, m_pDataRTF(NULL)
	, m_pDataHTML(NULL)
	, m_pDataDIB(NULL)
{
}

ClipboardData::~ClipboardData()
{
	FreeData();
}

void ClipboardData::FreeData()
{
	delete[] m_pDataText;
	delete[] m_pDataRTF;
	delete[] m_pDataHTML;
	delete[] m_pDataDIB;

	m_pDataText = NULL;
	m_pDataRTF = NULL;
	m_pDataHTML = NULL;
	m_pDataDIB = NULL;
}

static bool NormalizeDibTo24BppBI_RGB(const BYTE* pSrc, size_t srcLen, BYTE** ppOut, int* pOutLen)
{
	if (!pSrc || !ppOut || !pOutLen || srcLen < sizeof(BITMAPINFOHEADER)) {
		return false;
	}
	*ppOut = NULL;
	*pOutLen = 0;

	const BITMAPINFOHEADER* bih = (const BITMAPINFOHEADER*)pSrc;
	if (bih->biSize < sizeof(BITMAPINFOHEADER) || bih->biSize > srcLen) {
		return false;
	}
	if (bih->biWidth <= 0 || bih->biWidth > 16384) {
		return false;
	}
	int64_t absHeight = (bih->biHeight < 0) ? -(int64_t)bih->biHeight : (int64_t)bih->biHeight;
	if (absHeight <= 0 || absHeight > 16384) {
		return false;
	}

	uint32_t width = (uint32_t)bih->biWidth;
	uint32_t height = (uint32_t)absHeight;
	uint64_t dstStride = ((uint64_t)width * 3ULL + 3ULL) & ~3ULL;
	uint64_t dstPixelBytes = dstStride * (uint64_t)height;
	uint64_t dstTotal = sizeof(BITMAPINFOHEADER) + dstPixelBytes;
	if (dstTotal > 64ULL * 1024 * 1024) {
		return false;
	}

	// Fast path 1: already a clean bottom-up 24bpp BI_RGB BITMAPINFOHEADER
	if (bih->biSize == sizeof(BITMAPINFOHEADER) &&
		bih->biCompression == BI_RGB &&
		bih->biBitCount == 24 &&
		bih->biHeight > 0 &&
		bih->biPlanes == 1 &&
		bih->biClrUsed == 0 &&
		dstTotal <= srcLen) {
		BYTE* out = new BYTE[(size_t)dstTotal];
		memcpy(out, pSrc, (size_t)dstTotal);
		((BITMAPINFOHEADER*)out)->biSizeImage = (DWORD)dstPixelBytes;
		*ppOut = out;
		*pOutLen = (int)dstTotal;
		return true;
	}

	// Fast path 2: pure C++ 32bpp or top-down 24bpp BI_RGB BITMAPINFOHEADER
	if (bih->biSize == sizeof(BITMAPINFOHEADER) &&
		bih->biCompression == BI_RGB &&
		(bih->biBitCount == 24 || bih->biBitCount == 32) &&
		bih->biPlanes == 1 &&
		bih->biClrUsed == 0) {
		uint64_t bppBytes = (uint64_t)(bih->biBitCount / 8);
		uint64_t srcStride = ((uint64_t)width * bppBytes + 3ULL) & ~3ULL;
		uint64_t srcRequired = sizeof(BITMAPINFOHEADER) + srcStride * (uint64_t)height;
		if (srcRequired <= srcLen) {
			BYTE* out = new BYTE[(size_t)dstTotal];
			BITMAPINFOHEADER* outBih = (BITMAPINFOHEADER*)out;
			memset(outBih, 0, sizeof(BITMAPINFOHEADER));
			outBih->biSize = sizeof(BITMAPINFOHEADER);
			outBih->biWidth = (LONG)width;
			outBih->biHeight = (LONG)height;
			outBih->biPlanes = 1;
			outBih->biBitCount = 24;
			outBih->biCompression = BI_RGB;
			outBih->biSizeImage = (DWORD)dstPixelBytes;
			outBih->biXPelsPerMeter = bih->biXPelsPerMeter;
			outBih->biYPelsPerMeter = bih->biYPelsPerMeter;

			const BYTE* srcPixels = pSrc + sizeof(BITMAPINFOHEADER);
			BYTE* dstPixels = out + sizeof(BITMAPINFOHEADER);
			bool topDown = (bih->biHeight < 0);
			uint64_t rowCopyBytes = (uint64_t)width * 3ULL;

			for (uint32_t y = 0; y < height; ++y) {
				uint32_t srcY = topDown ? (height - 1 - y) : y;
				const BYTE* srcRow = srcPixels + (size_t)srcY * (size_t)srcStride;
				BYTE* dstRow = dstPixels + (size_t)y * (size_t)dstStride;
				if (bppBytes == 3) {
					memcpy(dstRow, srcRow, (size_t)rowCopyBytes);
				} else {
					for (uint32_t x = 0; x < width; ++x) {
						dstRow[x * 3 + 0] = srcRow[x * 4 + 0];
						dstRow[x * 3 + 1] = srcRow[x * 4 + 1];
						dstRow[x * 3 + 2] = srcRow[x * 4 + 2];
					}
				}
				if (dstStride > rowCopyBytes) {
					memset(dstRow + rowCopyBytes, 0, (size_t)(dstStride - rowCopyBytes));
				}
			}

			*ppOut = out;
			*pOutLen = (int)dstTotal;
			return true;
		}
	}

	// General GDI path: handles CF_DIBV5, BITMAPV4HEADER, BI_BITFIELDS, paletted, RLE, etc.
	size_t offset = bih->biSize;
	if (bih->biSize == sizeof(BITMAPINFOHEADER) && bih->biCompression == BI_BITFIELDS) {
		offset += 12;
	} else if (bih->biBitCount <= 8) {
		DWORD colors = bih->biClrUsed ? bih->biClrUsed : (1u << bih->biBitCount);
		offset += (size_t)colors * sizeof(RGBQUAD);
	} else if (bih->biClrUsed > 0) {
		offset += (size_t)bih->biClrUsed * sizeof(RGBQUAD);
	}
	if (bih->biSize >= sizeof(BITMAPV5HEADER)) {
		const BITMAPV5HEADER* v5 = (const BITMAPV5HEADER*)pSrc;
		if (v5->bV5ProfileData == offset && v5->bV5ProfileSize > 0) {
			offset += v5->bV5ProfileSize;
		}
	}
	if (offset >= srcLen) {
		return false;
	}

	BITMAPINFO dstBi;
	memset(&dstBi, 0, sizeof(dstBi));
	dstBi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	dstBi.bmiHeader.biWidth = (LONG)width;
	dstBi.bmiHeader.biHeight = (LONG)height;
	dstBi.bmiHeader.biPlanes = 1;
	dstBi.bmiHeader.biBitCount = 24;
	dstBi.bmiHeader.biCompression = BI_RGB;
	dstBi.bmiHeader.biSizeImage = (DWORD)dstPixelBytes;

	HDC hdc = ::GetDC(NULL);
	if (!hdc) {
		return false;
	}
	void* pDstBits = NULL;
	HBITMAP hDstBmp = ::CreateDIBSection(hdc, &dstBi, DIB_RGB_COLORS, &pDstBits, NULL, 0);
	HDC memDc = hDstBmp ? ::CreateCompatibleDC(hdc) : NULL;
	bool ok = false;

	if (hDstBmp && memDc && pDstBits) {
		HGDIOBJ oldObj = ::SelectObject(memDc, hDstBmp);
		int copied = ::StretchDIBits(
			memDc,
			0, 0, (int)width, (int)height,
			0, 0, (int)width, (int)height,
			pSrc + offset,
			(const BITMAPINFO*)pSrc,
			DIB_RGB_COLORS,
			SRCCOPY);
		::GdiFlush();
		if (copied > 0) {
			BYTE* out = new BYTE[(size_t)dstTotal];
			memcpy(out, &dstBi.bmiHeader, sizeof(BITMAPINFOHEADER));
			memcpy(out + sizeof(BITMAPINFOHEADER), pDstBits, (size_t)dstPixelBytes);
			*ppOut = out;
			*pOutLen = (int)dstTotal;
			ok = true;
		}
		::SelectObject(memDc, oldObj);
	}

	if (memDc) ::DeleteDC(memDc);
	if (hDstBmp) ::DeleteObject(hDstBmp);
	::ReleaseDC(NULL, hdc);
	return ok;
}

static bool ExtractDibFromHBitmap(HBITMAP hBmp, BYTE** ppOut, int* pOutLen)
{
	if (!hBmp || !ppOut || !pOutLen) {
		return false;
	}
	*ppOut = NULL;
	*pOutLen = 0;

	BITMAP bm;
	memset(&bm, 0, sizeof(bm));
	if (::GetObject(hBmp, sizeof(BITMAP), &bm) == 0) {
		return false;
	}
	if (bm.bmWidth <= 0 || bm.bmHeight <= 0 || bm.bmWidth > 16384 || bm.bmHeight > 16384) {
		return false;
	}

	uint32_t width = (uint32_t)bm.bmWidth;
	uint32_t height = (uint32_t)bm.bmHeight;
	uint64_t stride = ((uint64_t)width * 3ULL + 3ULL) & ~3ULL;
	uint64_t pixelBytes = stride * (uint64_t)height;
	uint64_t total = sizeof(BITMAPINFOHEADER) + pixelBytes;
	if (total > 64ULL * 1024 * 1024) {
		return false;
	}

	BITMAPINFO bi;
	memset(&bi, 0, sizeof(bi));
	bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bi.bmiHeader.biWidth = (LONG)width;
	bi.bmiHeader.biHeight = (LONG)height;
	bi.bmiHeader.biPlanes = 1;
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biSizeImage = (DWORD)pixelBytes;

	BYTE* out = new BYTE[(size_t)total];
	memset(out, 0, (size_t)total);
	memcpy(out, &bi.bmiHeader, sizeof(BITMAPINFOHEADER));

	HDC hdc = ::GetDC(NULL);
	int rows = 0;
	if (hdc) {
		rows = ::GetDIBits(hdc, hBmp, 0, (UINT)height, out + sizeof(BITMAPINFOHEADER), &bi, DIB_RGB_COLORS);
		::ReleaseDC(NULL, hdc);
	}

	if (rows > 0) {
		*ppOut = out;
		*pOutLen = (int)total;
		return true;
	}
	delete[] out;
	return false;
}

bool ClipboardData::Load(HWND hwndOwner) // will return false on failure
{
	// This can be improved by not making copies of the clipboard data and simply keeping the handles open; however
	// I seemed to notice some issues with having multiple clipboard format data handles open when an application 
	// uses clipboard formats that it must render on demand. Regardless, I am not too concerned about this now.

	ClipboardHolder holder(hwndOwner);

	if (!holder.m_bIsOpen) {
		return false;
	}

	m_crc = 0;
	FreeData();
	
	m_crc = crc32(0L, Z_NULL, 0);

	// text
	{
		m_lengthText = 0;

		HANDLE hText = NULL;
		if (IsClipboardFormatAvailable(ClipboardSettings::formatUnicodeText)) {
			hText = ::GetClipboardData(ClipboardSettings::formatUnicodeText);
		}

		if (hText) {
			BYTE* pData = (BYTE*)GlobalLock(hText);
			int nLength = (int)GlobalSize(hText);

			if (pData != NULL && nLength > 0) {

				// Convert from UTF-16 to UTF-8
				int nConvertedSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)pData, -1, NULL, 0, NULL, NULL);

				if (nConvertedSize > 0) {				
					m_pDataText = new BYTE[nConvertedSize];
					//memcpy(m_pDataText, pData, nLength);

					int nFinalConvertedSize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)pData, -1, (LPSTR)m_pDataText, nConvertedSize, NULL, NULL);

					if (nFinalConvertedSize > 0) {
						m_lengthText = nFinalConvertedSize;

						BYTE tag = 1;
						m_crc = crc32(m_crc, &tag, 1);
						m_crc = crc32(m_crc, m_pDataText, nFinalConvertedSize);
					} else {
						delete[] m_pDataText;
						m_pDataText = NULL;
					}
				}

				GlobalUnlock(hText);
			}
		}
	}

	// RTF
	{
		m_lengthRTF = 0;

		HANDLE hRTF = NULL;
		if (IsClipboardFormatAvailable(ClipboardSettings::formatRTF)) {
			hRTF = ::GetClipboardData(ClipboardSettings::formatRTF);
		}

		if (hRTF) {
			BYTE* pData = (BYTE*)GlobalLock(hRTF);
			int nLength = (int)GlobalSize(hRTF);

			if (pData != NULL && nLength > 0) {
				
				m_pDataRTF = new BYTE[nLength];
				memcpy(m_pDataRTF, pData, nLength);

				m_lengthRTF = nLength;
				BYTE tag = 2;
				m_crc = crc32(m_crc, &tag, 1);
				m_crc = crc32(m_crc, m_pDataRTF, nLength);

				GlobalUnlock(hRTF);
			}
		}
	}

	// HTML
	{
		m_lengthHTML = 0;

		HANDLE hHTML = NULL;
		if (IsClipboardFormatAvailable(ClipboardSettings::formatHTML)) {
			hHTML = ::GetClipboardData(ClipboardSettings::formatHTML);
		}

		if (hHTML) {
			BYTE* pData = (BYTE*)GlobalLock(hHTML);
			int nLength = (int)GlobalSize(hHTML);

			if (pData != NULL && nLength > 0) {
				
				m_pDataHTML = new BYTE[nLength];
				memcpy(m_pDataHTML, pData, nLength);

				m_lengthHTML = nLength;
				BYTE tag = 3;
				m_crc = crc32(m_crc, &tag, 1);
				m_crc = crc32(m_crc, m_pDataHTML, nLength);

				GlobalUnlock(hHTML);
			}
		}
	}

	// DIB (prefer CF_DIB, fall back to CF_BITMAP or CF_DIBV5, always normalize to 24bpp BI_RGB CF_DIB)
	{
		m_lengthDIB = 0;

		if (IsClipboardFormatAvailable(CF_DIB)) {
			HANDLE hDIB = ::GetClipboardData(CF_DIB);
			if (hDIB) {
				BYTE* pData = (BYTE*)GlobalLock(hDIB);
				SIZE_T nLength = GlobalSize(hDIB);
				if (pData != NULL && nLength > 0) {
					NormalizeDibTo24BppBI_RGB(pData, nLength, &m_pDataDIB, &m_lengthDIB);
					GlobalUnlock(hDIB);
				}
			}
		}

		if (m_pDataDIB == NULL && IsClipboardFormatAvailable(CF_BITMAP)) {
			HBITMAP hBmp = (HBITMAP)::GetClipboardData(CF_BITMAP);
			if (hBmp) {
				ExtractDibFromHBitmap(hBmp, &m_pDataDIB, &m_lengthDIB);
			}
		}

		if (m_pDataDIB == NULL && IsClipboardFormatAvailable(CF_DIBV5)) {
			HANDLE hDIB5 = ::GetClipboardData(CF_DIBV5);
			if (hDIB5) {
				BYTE* pData = (BYTE*)GlobalLock(hDIB5);
				SIZE_T nLength = GlobalSize(hDIB5);
				if (pData != NULL && nLength > 0) {
					NormalizeDibTo24BppBI_RGB(pData, nLength, &m_pDataDIB, &m_lengthDIB);
					GlobalUnlock(hDIB5);
				}
			}
		}

		if (m_lengthDIB > 0 && m_pDataDIB != NULL) {
			BYTE tag = 4;
			m_crc = crc32(m_crc, &tag, 1);
			m_crc = crc32(m_crc, m_pDataDIB, m_lengthDIB);
		}
	}

	return m_lengthText + m_lengthRTF + m_lengthHTML + m_lengthDIB > 0;
}

bool ClipboardData::Restore(HWND hwndOwner, ExtendedClipboardDataMessage& extendedClipboardDataMessage)
{
	ClipboardHolder holder(hwndOwner);

	if (!holder.m_bIsOpen) {
		return false;
	}

	if (!::EmptyClipboard()) {
		return false;
	}

	m_lengthText = m_lengthRTF = m_lengthHTML = m_lengthDIB = 0;

	int nCompressedDataLength = extendedClipboardDataMessage.GetBufferLength() - extendedClipboardDataMessage.GetDataLength();

	m_crc = crc32(0L, Z_NULL, 0);
	if (nCompressedDataLength == 0) {
		// no data beyond the flags
		return true;
	}

	rdr::MemInStream inStream(extendedClipboardDataMessage.GetCurrentPos(), nCompressedDataLength);
	rdr::ZlibInStream compressedStream;

	compressedStream.setUnderlying(&inStream, nCompressedDataLength);

	int length = 0;
	bool bFailed = false;
	if (extendedClipboardDataMessage.HasFlag(clipText)) {
		length = (int)compressedStream.readU32();

		if (length > 0 && length <= 104857600) {
			// get the incoming UTF-8 text
			BYTE* pIncomingText = new BYTE[length];
			compressedStream.readBytes(pIncomingText, length);

			BYTE tag = 1;
			m_crc = crc32(m_crc, &tag, 1);
			m_crc = crc32(m_crc, pIncomingText, length);

			// now we have to translate to UTF-16
			int nConvertedSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pIncomingText, length, NULL, 0);

			if (nConvertedSize > 0) {
				HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, nConvertedSize * sizeof(wchar_t));

				if (hData) {
					BYTE* pData = (BYTE*)GlobalLock(hData);

					if (pData) {
						int nFinalConvertedSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)pIncomingText, length, (LPWSTR)pData, nConvertedSize);

						GlobalUnlock(hData);

						if (nFinalConvertedSize > 0) {
							if (::SetClipboardData(ClipboardSettings::formatUnicodeText, hData)) {
								hData = NULL;
								m_lengthText = nConvertedSize * sizeof(wchar_t);
							} else {
								bFailed = true;
							}
						}
					}
				}

				if (hData) {
					GlobalFree(hData);
				}
			}
			if (pIncomingText) {
				delete[] pIncomingText;
			}
			if (bFailed) {
				return false;
			}
		}
	}

	if (extendedClipboardDataMessage.HasFlag(clipRTF)) {
		length = (int)compressedStream.readU32();

		if (length > 0 && length <= 104857600) {
			HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, length);

			if (!hData) return false;

			BYTE* pData = (BYTE*)GlobalLock(hData);

			if (pData) {
				compressedStream.readBytes(pData, length);

				BYTE tag = 2;
				m_crc = crc32(m_crc, &tag, 1);
				m_crc = crc32(m_crc, pData, length);

				GlobalUnlock(hData);

				if (::SetClipboardData(ClipboardSettings::formatRTF, hData)) {
					hData = NULL;
					m_lengthRTF = length;
				} else {
					bFailed = true;
				}
			}

			if (hData) {
				GlobalFree(hData);
			}
		}
	}
	
	if (extendedClipboardDataMessage.HasFlag(clipHTML)) {
		length = (int)compressedStream.readU32();

		if (length > 0 && length <= 104857600) {
			HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, length);

			if (!hData) return false;

			BYTE* pData = (BYTE*)GlobalLock(hData);

			if (pData) {
				compressedStream.readBytes(pData, length);

				BYTE tag = 3;
				m_crc = crc32(m_crc, &tag, 1);
				m_crc = crc32(m_crc, pData, length);

				GlobalUnlock(hData);

				if (::SetClipboardData(ClipboardSettings::formatHTML, hData)) {
					hData = NULL;
					m_lengthHTML = length;
				} else {
					bFailed = true;
				}
			}

			if (hData) {
				GlobalFree(hData);
			}
		}
	}

	if (extendedClipboardDataMessage.HasFlag(clipDIB)) {
		length = (int)compressedStream.readU32();

		if (length > 0 && length <= 104857600) {
			std::vector<BYTE> rawDib(length);
			compressedStream.readBytes(rawDib.data(), length);

			BYTE tag = 4;
			m_crc = crc32(m_crc, &tag, 1);
			m_crc = crc32(m_crc, rawDib.data(), length);

			BYTE* pNormDib = NULL;
			int normLen = 0;
			const BYTE* pOutDib = rawDib.data();
			int outLen = length;
			UINT clipFmt = CF_DIB;

			if (NormalizeDibTo24BppBI_RGB(rawDib.data(), rawDib.size(), &pNormDib, &normLen)) {
				pOutDib = pNormDib;
				outLen = normLen;
				clipFmt = CF_DIB;
			} else if (length >= (int)sizeof(BITMAPV5HEADER) &&
				((const BITMAPINFOHEADER*)rawDib.data())->biSize == sizeof(BITMAPV5HEADER)) {
				clipFmt = CF_DIBV5;
			}

			HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, outLen);
			if (hData) {
				BYTE* pData = (BYTE*)GlobalLock(hData);
				if (pData) {
					memcpy(pData, pOutDib, outLen);
					GlobalUnlock(hData);
					if (::SetClipboardData(clipFmt, hData)) {
						hData = NULL;
						m_lengthDIB = outLen;
					} else {
						bFailed = true;
					}
				}
				if (hData) {
					GlobalFree(hData);
				}
			} else {
				bFailed = true;
			}

			delete[] pNormDib;
		}
	}

	// we can ignore everything else

	return !bFailed;
}

Clipboard::Clipboard(CARD32 caps)
	: settings(caps)
	, m_crc(0)
	, m_bNeedToProvide(false)
	, m_bNeedToNotify(false)
	, m_notifiedRemoteFormats(0)
{
}

// returns true if something changed
bool Clipboard::UpdateClipTextEx(ClipboardData& clipboardData, CARD32 overrideFlags)
{
	if (m_crc == clipboardData.m_crc && overrideFlags == 0) {
		return false;
	}

	if (overrideFlags & clipPeek) {
		// don't reset anything regarding providing new data, just 'include' any
		// formats into the notify response.
		if (clipboardData.m_lengthText != 0) {
			extendedClipboardDataNotifyMessage.AddFlag(clipText);
			m_bNeedToNotify = true;
		}
		if (clipboardData.m_lengthRTF != 0) {
			extendedClipboardDataNotifyMessage.AddFlag(clipRTF);
			m_bNeedToNotify = true;
		}
		if (clipboardData.m_lengthHTML != 0) {
			extendedClipboardDataNotifyMessage.AddFlag(clipHTML);
			m_bNeedToNotify = true;
		}
		if (clipboardData.m_lengthDIB != 0) {
			extendedClipboardDataNotifyMessage.AddFlag(clipDIB);
			m_bNeedToNotify = true;
		}
	} else {

		m_bNeedToProvide = false;
		m_bNeedToNotify = false;
		extendedClipboardDataMessage.Reset();
		extendedClipboardDataNotifyMessage.Reset();

		extendedClipboardDataMessage.AddFlag(clipProvide);
		extendedClipboardDataNotifyMessage.AddFlag(clipNotify);

		rdr::MemOutStream memStream;
		{
			int compLevel = (clipboardData.m_lengthDIB > 0) ? 1 : 3;
			rdr::ZlibOutStream compressedStream(&memStream, 0, compLevel);

			if (clipboardData.m_lengthText != 0) {
				extendedClipboardDataNotifyMessage.AddFlag(clipText);

				if (clipboardData.m_lengthText <= settings.m_nLimitText || (overrideFlags & clipText)) {
					compressedStream.writeU32(clipboardData.m_lengthText);
					compressedStream.writeBytes(clipboardData.m_pDataText, clipboardData.m_lengthText);
					extendedClipboardDataMessage.AddFlag(clipText);
					m_bNeedToProvide = true;
				} else if (!(overrideFlags & clipRequest)){
					m_bNeedToNotify = true;
				}
			}
			if (clipboardData.m_lengthRTF != 0) {
				extendedClipboardDataNotifyMessage.AddFlag(clipRTF);

				if (clipboardData.m_lengthRTF <= settings.m_nLimitRTF || (overrideFlags & clipRTF)) {
					compressedStream.writeU32(clipboardData.m_lengthRTF);
					compressedStream.writeBytes(clipboardData.m_pDataRTF, clipboardData.m_lengthRTF);
					extendedClipboardDataMessage.AddFlag(clipRTF);
					m_bNeedToProvide = true;
				} else if (!(overrideFlags & clipRequest)) {
					m_bNeedToNotify = true;
				}
			}
			if (clipboardData.m_lengthHTML != 0) {
				extendedClipboardDataNotifyMessage.AddFlag(clipHTML);

				if (clipboardData.m_lengthHTML <= settings.m_nLimitHTML || (overrideFlags & clipHTML)) {
					compressedStream.writeU32(clipboardData.m_lengthHTML);
					compressedStream.writeBytes(clipboardData.m_pDataHTML, clipboardData.m_lengthHTML);
					extendedClipboardDataMessage.AddFlag(clipHTML);
					m_bNeedToProvide = true;
				} else if (!(overrideFlags & clipRequest)) {
					m_bNeedToNotify = true;
				}
			}
			if (clipboardData.m_lengthDIB != 0) {
				extendedClipboardDataNotifyMessage.AddFlag(clipDIB);

				if (clipboardData.m_lengthDIB <= settings.m_nLimitDIB || (overrideFlags & clipDIB)) {
					compressedStream.writeU32(clipboardData.m_lengthDIB);
					compressedStream.writeBytes(clipboardData.m_pDataDIB, clipboardData.m_lengthDIB);
					extendedClipboardDataMessage.AddFlag(clipDIB);
					m_bNeedToProvide = true;
				} else if (!(overrideFlags & clipRequest)) {
					m_bNeedToNotify = true;
				}
			}

			compressedStream.flush();
		}

		if (m_bNeedToProvide) {
			extendedClipboardDataMessage.AppendBytes((BYTE*)memStream.data(), memStream.length());
			memStream.clear();
		}

		m_strLastCutText = L"";
		if (!settings.m_bSupportsEx && clipboardData.m_lengthText > 0 && clipboardData.m_lengthText < settings.m_nLimitText) {			
			// Convert UTF-8 to UTF-16 and store directly (no CP_ACP conversion needed)
			int nConvertedSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)clipboardData.m_pDataText, clipboardData.m_lengthText, NULL, 0);

			if (nConvertedSize > 0) {
				wchar_t* clipStr = new wchar_t[nConvertedSize + 1];
				int nFinalConvertedSize = MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)clipboardData.m_pDataText, clipboardData.m_lengthText, (LPWSTR)clipStr, nConvertedSize);

				if (nFinalConvertedSize > 0) {
					clipStr[nFinalConvertedSize] = L'\0';
					// Store Unicode directly - no CP_ACP conversion, preserves all characters
					m_strLastCutText.assign(clipStr, nFinalConvertedSize);
				}

				delete[] clipStr;
			}
		}

		m_crc = clipboardData.m_crc;

		if ( (!(settings.m_remoteCaps & clipNotify)) || (overrideFlags != 0) ) {
			m_bNeedToNotify = false;
			extendedClipboardDataNotifyMessage.Reset();
		}
	}

	return m_bNeedToProvide || m_bNeedToNotify;
}

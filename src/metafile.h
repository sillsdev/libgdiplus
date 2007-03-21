/*
 * Copyright (C) 2007 Novell, Inc (http://www.novell.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 * NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	Sebastien Pouliot  <sebastien@ximian.com>
 */

#ifndef __METAFILE_H__
#define __METAFILE_H__

#include <stdio.h>
#include <math.h>
#include "gdip.h"
#include "gdipImage.h"
#include "dstream.h"
#include "brush.h"
#include "solidbrush.h"

/*
 *	http://wvware.sourceforge.net/caolan/ora-wmf.html
 */

/* definitions */

#define ALDUS_PLACEABLE_METAFILE_KEY	0x9AC6CDD7
#define WMF_TYPE_AND_HEADERSIZE_KEY	0x00090001
#define EMF_EMR_HEADER_KEY		0x1

/* this has to do with 25.4mm in an inch (but why is it multiplied by 100 ?) */
#define METAFILE_DIMENSION_FACTOR	2540
#define MM_PER_INCH			25.4f

/* match System.Drawing.Imaging.MetafileType */
#define METAFILETYPE_INVALID		0
#define METAFILETYPE_WMF		1
#define METAFILETYPE_WMFPLACEABLE	2
#define METAFILETYPE_EMF		3
#define METAFILETYPE_EMFPLUSONLY	4
#define METAFILETYPE_EMFPLUSDUAL	5

/* object types */
#define METAOBJECT_TYPE_EMPTY	0
#define METAOBJECT_TYPE_PEN	1
#define METAOBJECT_TYPE_BRUSH	2

/* SetBkMode */
#define TRANSPARENT		1
#define OPAQUE			2

/* SetMapMode */
#define MM_TEXT			1
#define MM_LOMETRIC		2
#define MM_HIMETRIC		3
#define MM_LOENGLISH		4
#define MM_HIENGLISH		5
#define MM_TWIPS		6
#define MM_ISOTROPIC		7
#define MM_ANISOTROPIC		8

/* CreatePenIndirect */
#define PS_NULL			0x00000005
#define PS_STYLE_MASK		0x0000000F
#define PS_ENDCAP_ROUND		0x00000000
#define PS_ENDCAP_SQUARE	0x00000100
#define PS_ENDCAP_FLAT		0x00000200
#define PS_ENDCAP_MASK		0x00000F00
#define PS_JOIN_ROUND		0x00000000
#define PS_JOIN_BEVEL		0x00001000
#define PS_JOIN_MITER		0x00002000
#define PS_JOIN_MASK		0x0000F000

/* CreateBrushIndirect */
#define BS_SOLID		0
#define BS_NULL			1
#define BS_HATCHED		2
#define BS_PATTERN		3
#define BS_INDEXED		4

/* SetPolyFillMode */
#define ALTERNATE		1
#define WINDING			2

/* SetRelabs */
#define ABSOLUTE		1
#define RELATIVE		2

typedef struct {
	LONG	x;
	LONG	y;
} POINT;

typedef DWORD COLORREF;

typedef struct {
	UINT		lopnStyle;
	POINT		lopnWidth;
	COLORREF	lopnColor;
} LOGPEN;

typedef struct {
	UINT		lbStyle;
	COLORREF	lbColor;
	LONG		lbHatch;
} LOGBRUSH;

#pragma pack(2)

typedef struct {
	int	left;
	int	top;
	int	right;
	int	bottom;
} RECT, RECTL;

typedef struct {
	int	cx;
	int	cy; 
} SIZE, SIZEL;

typedef struct {
	WORD	mtType;			/* 1 for disk, 0 for memory */
	WORD	mtHeaderSize;
	WORD	mtVersion;
	DWORD	mtSize;
	WORD	mtNoObjects;
	DWORD	mtMaxRecord;
	WORD	mtNoParameters;
} METAHEADER;

typedef struct {
	DWORD	iType;
	DWORD	nSize;
	RECTL	rclBounds;
	RECTL	rclFrame;
	DWORD	dSignature;
	DWORD	nVersion;
	DWORD	nBytes;
	DWORD	nRecords;
	WORD	nHandles;
	WORD	sReserved;
	DWORD	nDescription;
	DWORD	offDescription;
	DWORD	nPalEntries;
	SIZEL	szlDevice;
	SIZEL	szlMillimeters;
} ENHMETAHEADER3;

typedef struct {
	SHORT	Left;
	SHORT	Top;
	SHORT	Right;
	SHORT	Bottom;
} PWMFRect16;

typedef struct {
	DWORD		Key;
	SHORT		Hmf;
	PWMFRect16	BoundingBox;
	SHORT		Inch;
	DWORD		Reserved;
	SHORT		Checksum;
} WmfPlaceableFileHeader;

typedef struct {
	int	Type;
	int	Size;
	int	Version;
	int	EmfPlusFlags;
	float	DpiX;
	float	DpiY;
	int	X;
	int	Y;
	int	Width;
	int	Height;
	union {
		METAHEADER	WmfHeader;
		ENHMETAHEADER3	EmfHeader;
	};
	int	EmfPlusHeaderSize;
	int	LogicalDpiX;
	int	LogicalDpiY;
} MetafileHeader;

typedef struct {
	void *ptr;
	int type;
} MetaObject;

typedef struct {
	GpImage base;
	MetafileHeader metafile_header;
	BOOL delete;
	BYTE *data;
	int length;
} GpMetafile;

typedef struct {
	GpMetafile *metafile;
	int x, y, width, height;
	int objects_count;
	MetaObject *objects;
	MetaObject created;
	GpGraphics *graphics;
	GpMatrix matrix;
	DWORD bk_mode;
	DWORD bk_color;
	float miter_limit;
	int selected_pen;
	int selected_brush;
	int selected_font;
	int selected_palette;
	int map_mode;
	GpFillMode fill_mode;
	int current_x, current_y;
	/* path related data */
	BOOL use_path;
	GpPath *path;
	int path_x, path_y;
	/* stock objects */
	GpPen *stock_pen_white;
	GpPen *stock_pen_black;
	GpPen *stock_pen_null;
	GpSolidFill *stock_brush_white;
	GpSolidFill *stock_brush_ltgray;
	GpSolidFill *stock_brush_gray;
	GpSolidFill *stock_brush_dkgray;
	GpSolidFill *stock_brush_black;
	GpSolidFill *stock_brush_null;
	/* bitmap representation */
	BYTE *scan0;
} MetafilePlayContext, HDC;

typedef struct {
	int num;
	GpPointF *points;
} PointFList;

typedef gpointer HMETAFILE;
typedef gpointer HENHMETAFILE;

typedef enum {
	WmfRecordType = 0
} EmfPlusRecordType;

#pragma pack()

/* function prototypes */

GpStatus GdipCreateMetafileFromWmf (HMETAFILE hWmf, BOOL deleteWmf, GDIPCONST WmfPlaceableFileHeader *wmfPlaceableFileHeader, GpMetafile **metafile);

GpStatus GdipCreateMetafileFromEmf (HENHMETAFILE hEmf, BOOL deleteEmf, GpMetafile **metafile);

GpStatus GdipCreateMetafileFromFile (GDIPCONST WCHAR *file, GpMetafile **metafile);

GpStatus GdipCreateMetafileFromWmfFile (GDIPCONST WCHAR *file, GDIPCONST WmfPlaceableFileHeader *wmfPlaceableFileHeader, GpMetafile **metafile);

GpStatus GdipCreateMetafileFromStream (void *stream, GpMetafile **metafile);
GpStatus GdipCreateMetafileFromDelegate_linux (GetHeaderDelegate getHeaderFunc, GetBytesDelegate getBytesFunc,
	PutBytesDelegate putBytesFunc, SeekDelegate seekFunc, CloseDelegate closeFunc, SizeDelegate sizeFunc, 
	GpMetafile **metafile);

GpStatus GdipGetMetafileHeaderFromWmf (HMETAFILE hWmf, GDIPCONST WmfPlaceableFileHeader *wmfPlaceableFileHeader, MetafileHeader *header);

GpStatus GdipGetMetafileHeaderFromEmf (HENHMETAFILE hEmf, MetafileHeader *header);

GpStatus GdipGetMetafileHeaderFromFile (GDIPCONST WCHAR *filename, MetafileHeader *header);

GpStatus GdipGetMetafileHeaderFromStream (void *stream, MetafileHeader *header);
GpStatus GdipGetMetafileHeaderFromDelegate_linux (GetHeaderDelegate getHeaderFunc, GetBytesDelegate getBytesFunc,
	PutBytesDelegate putBytesFunc, SeekDelegate seekFunc, CloseDelegate closeFunc, SizeDelegate sizeFunc, 
	MetafileHeader *header);

GpStatus GdipGetMetafileHeaderFromMetafile (GpMetafile *metafile, MetafileHeader *header);

GpStatus GdipGetHemfFromMetafile (GpMetafile *metafile, HENHMETAFILE *hEmf);

GpStatus GdipGetMetafileDownLevelRasterizationLimit (GpMetafile *metafile, UINT *metafileRasterizationLimitDpi);

GpStatus GdipSetMetafileDownLevelRasterizationLimit (GpMetafile *metafile, UINT metafileRasterizationLimitDpi);

GpStatus GdipPlayMetafileRecord (GDIPCONST GpMetafile *metafile, EmfPlusRecordType recordType, UINT flags, UINT dataSize, GDIPCONST BYTE* data);

/* internal functions */

#define gdip_get_metaheader(image)	(&((GpMetafile*)image)->metafile_header)

GpStatus gdip_get_metafile_from (void *pointer, GpMetafile **metafile, bool useFile);
GpStatus gdip_metafile_clone (GpMetafile *metafile, GpMetafile **clonedmetafile);
GpStatus gdip_metafile_dispose (GpMetafile *metafile);

GpStatus gdip_metafile_play_emf (MetafilePlayContext *context);
GpStatus gdip_metafile_play_wmf (MetafilePlayContext *context);

MetafilePlayContext* gdip_metafile_play_setup (GpMetafile *metafile, GpGraphics *graphics, int x, int y, int width, int height);
GpStatus gdip_metafile_play (MetafilePlayContext *context);
GpStatus gdip_metafile_play_cleanup (MetafilePlayContext *context);

GpPen* gdip_metafile_GetSelectedPen (MetafilePlayContext *context);
GpBrush* gdip_metafile_GetSelectedBrush (MetafilePlayContext *context);

GpStatus gdip_metafile_SetBkMode (MetafilePlayContext *context, DWORD bkMode);
GpStatus gdip_metafile_SetMapMode (MetafilePlayContext *context, DWORD mode);
GpStatus gdip_metafile_SetROP2 (MetafilePlayContext *context, DWORD rop);
GpStatus gdip_metafile_SetRelabs (MetafilePlayContext *context, DWORD mode);
GpStatus gdip_metafile_SetPolyFillMode (MetafilePlayContext *context, DWORD mode);
GpStatus gdip_metafile_SelectObject (MetafilePlayContext *context, DWORD slot);
GpStatus gdip_metafile_SetTextAlign (MetafilePlayContext *context, DWORD textalign);
GpStatus gdip_metafile_DeleteObject (MetafilePlayContext *context, DWORD slot);
GpStatus gdip_metafile_SetBkColor (MetafilePlayContext *context, DWORD color);
GpStatus gdip_metafile_SetWindowOrg (MetafilePlayContext *context, int x, int y);
GpStatus gdip_metafile_SetWindowExt (MetafilePlayContext *context, int height, int width);
GpStatus gdip_metafile_LineTo (MetafilePlayContext *context, int x, int y);
GpStatus gdip_metafile_MoveTo (MetafilePlayContext *context, int x, int y);
GpStatus gdip_metafile_SetMiterLimit (MetafilePlayContext *context, float eNewLimit, float *peOldLimit);
GpStatus gdip_metafile_CreatePenIndirect (MetafilePlayContext *context, DWORD style, DWORD width, DWORD color);
GpStatus gdip_metafile_ExtCreatePen (MetafilePlayContext *context, DWORD dwPenStyle, DWORD dwWidth, CONST LOGBRUSH *lplb,
	DWORD dwStyleCount, CONST DWORD *lpStyle);
GpStatus gdip_metafile_CreateBrushIndirect (MetafilePlayContext *context, DWORD style, DWORD color, DWORD hatch);
GpStatus gdip_metafile_Arc (MetafilePlayContext *context, int left, int top, int right, int bottom, 
	int xstart, int ystart, int xend, int yend);
GpStatus gdip_metafile_PolyBezier (MetafilePlayContext *context, GpPointF *points, int count);
GpStatus gdip_metafile_Polygon (MetafilePlayContext *context, GpPointF *points, int count);
GpStatus gdip_metafile_BeginPath (MetafilePlayContext *context);
GpStatus gdip_metafile_EndPath (MetafilePlayContext *context);
GpStatus gdip_metafile_CloseFigure (MetafilePlayContext *context);
GpStatus gdip_metafile_FillPath (MetafilePlayContext *context);
GpStatus gdip_metafile_StrokePath (MetafilePlayContext *context);
GpStatus gdip_metafile_StrokeAndFillPath (MetafilePlayContext *context);

#endif
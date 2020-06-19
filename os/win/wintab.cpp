// LAF OS Library
// Copyright (C) 2020  Igara Studio S.A.
// Copyright (C) 2016-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "os/win/wintab.h"

#include "base/convert_to.h"
#include "base/debug.h"
#include "base/fs.h"
#include "base/log.h"
#include "base/sha1.h"
#include "base/string.h"
#include "base/version.h"

#include <iostream>
#include <algorithm>

#define WINTAB_TRACE(...)

namespace os {

namespace {

typedef UINT (API* WTInfoW_Func)(UINT, UINT, LPVOID);
typedef HCTX (API* WTOpenW_Func)(HWND, LPLOGCONTEXTW, BOOL);
typedef BOOL (API* WTClose_Func)(HCTX);
typedef int (API* WTPacketsGet_Func)(HCTX, int, LPVOID);
typedef BOOL (API* WTPacket_Func)(HCTX, UINT, LPVOID);
typedef BOOL (API* WTOverlap_Func)(HCTX, BOOL);
typedef int (API* WTQueueSizeGet_Func)(HCTX);
typedef BOOL (API* WTQueueSizeSet_Func)(HCTX, int);

WTInfoW_Func WTInfo;
WTOpenW_Func WTOpen;
WTClose_Func WTClose;
WTPacketsGet_Func WTPacketsGet;
WTPacket_Func WTPacket;
WTOverlap_Func WTOverlap;
WTQueueSizeGet_Func WTQueueSizeGet;
WTQueueSizeSet_Func WTQueueSizeSet;

} // anonymous namespace

WintabAPI::WintabAPI()
  : m_wintabLib(nullptr)
{
}

WintabAPI::~WintabAPI()
{
  if (!m_wintabLib)
    return;

  base::unload_dll(m_wintabLib);
  m_wintabLib = nullptr;
}

HCTX WintabAPI::open(HWND hwnd, bool moveMouse)
{
  if (!m_wintabLib && !loadWintab())
    return nullptr;

  // Log Wintab ID
  {
    UINT nchars = WTInfo(WTI_INTERFACE, IFC_WINTABID, nullptr);
    if (nchars > 0 && nchars < 1024) {
      // Some buggy wintab implementations may not report the right string size in the WTInfo call above (eg.: the Genius EasyPen i405X wintab). When this happens, the WTInfo call for getting the tablet id will get only a part of the string, therefore without the null terminating character. On windows this will lead to a failure when msvc free implementation tries to dealocate the storage for the std::vector used to hold the string. This may just crash instantly, or cause a heap corruption that will lead to a crash latter. A quick workaround to this kind of wintab misinformation is to oversize the buffer to guarantee that for the most common string lenghts it will be enough.
      std::vector<WCHAR> buf(std::min(128, nchars+1), 0);
      WTInfo(WTI_INTERFACE, IFC_WINTABID, &buf[0]);
      LOG("PEN: Wintab ID \"%s\"\n", base::to_utf8(&buf[0]).c_str());
    }
  }

  // Log Wintab version for debugging purposes
  {
    WORD specVer = 0;
    WORD implVer = 0;
    UINT options = 0;
    WTInfo(WTI_INTERFACE, IFC_SPECVERSION, &specVer);
    WTInfo(WTI_INTERFACE, IFC_IMPLVERSION, &implVer);
    WTInfo(WTI_INTERFACE, IFC_CTXOPTIONS, &options);
    LOG("PEN: Wintab spec v%d.%d impl v%d.%d options 0x%x\n",
        (specVer & 0xff00) >> 8, (specVer & 0xff),
        (implVer & 0xff00) >> 8, (implVer & 0xff), options);
  }

  LOGCONTEXTW logctx;
  memset(&logctx, 0, sizeof(LOGCONTEXTW));
  UINT infoRes = WTInfo(WTI_DEFSYSCTX, 0, &logctx);

  if (moveMouse) {
    // Move system cursor position, you can use packets to get
    // pressure information and cursor type, and pointer movement from
    // mouse messages.
    logctx.lcOptions |= CXO_SYSTEM;
  }
  else {
    // In this case you can process packets directly converting then
    // to events (system mouse movement messages will not be
    // generated).
    logctx.lcOptions &= ~CXO_SYSTEM;
  }

#if 1 // We shouldn't bypass WTOpen() if the return value from
      // WTInfo() isn't the expected one, WTOpen() should just fail
      // anyway.
  if (infoRes != sizeof(LOGCONTEXTW)) {
    LOG(ERROR,
        "PEN: Invalid size of WTInfo:\n"
        "     Expected context size: %d\n"
        "     Actual context size: %d\n",
        sizeof(LOGCONTEXTW), infoRes);
  }
#endif

  LOG("PEN: Context options=%d pktRate=%d in=%d,%d,%d,%d out=%d,%d,%d,%d sys=%d,%d,%d,%d\n",
      logctx.lcOptions, logctx.lcPktRate,
      logctx.lcInOrgX, logctx.lcInOrgY, logctx.lcInExtX, logctx.lcInExtY,
      logctx.lcOutOrgX, logctx.lcOutOrgY, logctx.lcOutExtX, logctx.lcOutExtY,
      logctx.lcSysOrgX, logctx.lcSysOrgY, logctx.lcSysExtX, logctx.lcSysExtY);

  logctx.lcOptions |=
    CXO_MESSAGES |
    CXO_CSRMESSAGES;
  logctx.lcPktData = PACKETDATA;
  logctx.lcPktMode = PACKETMODE;
  logctx.lcMoveMask = PACKETDATA;
  m_outBounds = gfx::Rect(logctx.lcOutOrgX, logctx.lcOutOrgY, logctx.lcOutExtX, logctx.lcOutExtY);

  AXIS pressure;
  infoRes = WTInfo(WTI_DEVICES, DVC_NPRESSURE, &pressure);
  if (infoRes >= sizeof(AXIS)) {
    m_minPressure = pressure.axMin;
    m_maxPressure = pressure.axMax;
    LOG("PEN: Min/max pressure values [%d,%d]\n", pressure.axMin, pressure.axMax);
  }
  else {
    m_minPressure = 0;
    m_maxPressure = 0;
    LOG("PEN: pressure info size %d (expected %d)", infoRes, sizeof(AXIS));
  }

  LOG("PEN: Opening context, options 0x%x\n", logctx.lcOptions);
  HCTX ctx = WTOpen(hwnd, &logctx, TRUE);
  if (!ctx) {
    LOG("PEN: Error attaching pen to display\n");
    return nullptr;
  }

  // Make the queue bigger as recommended by Wacom docs:
  //
  //   "To prevent your queue from overflowing, increase the size of
  //    your context's queue with the WTQueueSizeSet() function to a
  //    value between 32 and 128. Memory for packet queues is a limited
  //    resource, so be sure check that your call to WTQueueSizeSet()
  //    succeeds. If WTQueueSizeSet() fails, request a smaller queue
  //    size."
  //
  // https://developer-docs.wacom.com/display/DevDocs/Window+Developers+FAQ
  int q = WTQueueSizeGet(ctx);
  LOG("PEN: Original queue size=%d\n", q);
  if (q < 128) {
    for (int r=128; r>=q; r-=8) {
      if (WTQueueSizeSet(ctx, r))
        break;
    }
  }
  m_queueSize = q = WTQueueSizeGet(ctx);
  LOG("PEN: New queue size=%d\n", q);

  LOG("PEN: Pen attached to display, new context %p\n", ctx);
  return ctx;
}

void WintabAPI::close(HCTX ctx)
{
  LOG("PEN: Closing context %p\n", ctx);
  if (ctx) {
    ASSERT(m_wintabLib);
    LOG("PEN: Pen detached from window\n");
    WTClose(ctx);
  }
}

void WintabAPI::overlap(HCTX ctx, BOOL state)
{
  WTOverlap(ctx, state);
}

bool WintabAPI::packet(HCTX ctx, UINT serial, LPVOID packet)
{
  return (WTPacket(ctx, serial, packet) ? true: false);
}

int WintabAPI::packets(HCTX ctx, int maxPackets, LPVOID packets)
{
  return WTPacketsGet(ctx, maxPackets, packets);
}

void WintabAPI::mapCursorButton(const int cursor,
                                const int logicalButton,
                                const int relativeButton,
                                Event::Type& evType,
                                Event::MouseButton& mouseButton)
{
  mouseButton = Event::NoneButton;
  switch (relativeButton) {
    case TBN_DOWN:
      evType = Event::MouseDown;
      break;
    case TBN_UP:
      evType = Event::MouseUp;
      break;
    case TBN_NONE:
    default:
      evType = Event::MouseMove;
      break;
  }

  // Invalid logical button
  if (logicalButton < 0 || logicalButton >= 32) {
    WINTAB_TRACE("PEN: INVALID LOGICAL BUTTON\n");
    return;
  }

  // Get "logical button" -> "button action code" mapping so we can
  // know for what specific mouse button we should generate an event
  // (or maybe if it's a double-click).
  BYTE map[32];
  WTInfo(WTI_CURSORS + cursor, CSR_SYSBTNMAP, &map);

  switch (map[logicalButton]) {

    case SBN_LDBLCLICK:
      evType = Event::MouseDoubleClick;
    case SBN_LCLICK:
    case SBN_LDRAG:
      mouseButton = Event::LeftButton;
      break;

    case SBN_RDBLCLICK:
      evType = Event::MouseDoubleClick;
    case SBN_RCLICK:
    case SBN_RDRAG:
      mouseButton = Event::RightButton;
      break;

    case SBN_MDBLCLICK:
      evType = Event::MouseDoubleClick;
    case SBN_MCLICK:
    case SBN_MDRAG:
      mouseButton = Event::MiddleButton;
      break;
  }

  WINTAB_TRACE(
    "  PEN: Button map logicalButton=%d action=%d -> evType=%s mouseButton=%d\n",
    logicalButton,
    map[logicalButton],
    (evType == Event::None ? "-":
     evType == Event::MouseMove ? "move":
     evType == Event::MouseDown ? "DOWN":
     evType == Event::MouseUp ? "UP":
     evType == Event::MouseDoubleClick ? "DOUBLE-CLICK": "unknown"),
    (int)mouseButton);
}

bool WintabAPI::loadWintab()
{
  ASSERT(!m_wintabLib);

  m_wintabLib = base::load_dll("wintab32.dll");
  if (!m_wintabLib) {
    LOG(ERROR, "PEN: wintab32.dll is not present\n");
    return false;
  }

  if (!checkDll()) {
    base::unload_dll(m_wintabLib);
    m_wintabLib = nullptr;
    return false;
  }

  WTInfo = base::get_dll_proc<WTInfoW_Func>(m_wintabLib, "WTInfoW");
  WTOpen = base::get_dll_proc<WTOpenW_Func>(m_wintabLib, "WTOpenW");
  WTClose = base::get_dll_proc<WTClose_Func>(m_wintabLib, "WTClose");
  WTPacketsGet = base::get_dll_proc<WTPacketsGet_Func>(m_wintabLib, "WTPacketsGet");
  WTPacket = base::get_dll_proc<WTPacket_Func>(m_wintabLib, "WTPacket");
  WTOverlap = base::get_dll_proc<WTOverlap_Func>(m_wintabLib, "WTOverlap");
  WTQueueSizeGet = base::get_dll_proc<WTQueueSizeGet_Func>(m_wintabLib, "WTQueueSizeGet");
  WTQueueSizeSet = base::get_dll_proc<WTQueueSizeSet_Func>(m_wintabLib, "WTQueueSizeSet");
  if (!WTInfo || !WTOpen || !WTClose || !WTPacket ||
      !WTQueueSizeGet || !WTQueueSizeSet) {
    LOG(ERROR, "PEN: wintab32.dll does not contain all required functions\n");
    return false;
  }

  LOG("PEN: Wintab library loaded\n");
  return true;
}

bool WintabAPI::checkDll()
{
  ASSERT(m_wintabLib);

  std::string fn = base::get_dll_filename(m_wintabLib);
  if (!base::is_file(fn))
    return false;

  std::string checksum = base::convert_to<std::string>(base::Sha1::calculateFromFile(fn));
  base::Version ver = base::get_file_version(fn);
  LOG("PEN: <%s> v%s, sha1 <%s>\n", fn.c_str(), ver.str().c_str(), checksum.c_str());

  // Ugly hack to bypass the buggy WALTOP International Corp .dll that
  // hangs Aseprite completely when we call its WTInfo function.
  if (checksum == "a3ba0d9c0f5d8b9f4070981b243a80579f8be105")
    return false;

  return true;
}

} // namespace os

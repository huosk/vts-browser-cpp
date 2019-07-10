/**
 * Copyright (c) 2017 Melown Technologies SE
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * *  Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// THIS FILE IS GENERATED BY SCRIPT
// DO NOT MODIFY

using System;
using System.Runtime.InteropServices;

namespace vts
{
	public static class BrowserInterop
	{

#if ENABLE_IL2CPP
	const string LibName = "__Internal";
#else
	const string LibName = "vts-browser";
#endif

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void vtsResourceCallbackType(IntPtr map, IntPtr resource);

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void vtsMapCallbackType(IntPtr map);

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate IntPtr  vtsProjFinderCallbackType([MarshalAs(UnmanagedType.LPStr)] string name);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCallbacksLoadTexture(IntPtr map, vtsResourceCallbackType callback);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCallbacksLoadMesh(IntPtr map, vtsResourceCallbackType callback);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCallbacksMapconfigAvailable(IntPtr map, vtsMapCallbackType callback);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCallbacksMapconfigReady(IntPtr map, vtsMapCallbackType callback);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCallbacksProjFinder(vtsProjFinderCallbackType callback);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsCameraCreate(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraDestroy(IntPtr cam);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraSetViewportSize(IntPtr cam, uint width, uint height);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraSetView(IntPtr cam, [In] double[] eye, [In] double[] target, [In] double[] up);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraSetViewMatrix(IntPtr cam, [In] double[] view);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraSetProj(IntPtr cam, double fovyDegs, double near_, double far_);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraSetProjMatrix(IntPtr cam, [In] double[] proj);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraGetViewportSize(IntPtr cam, ref uint width, ref uint height);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraGetView(IntPtr cam, [Out] double[] eye, [Out] double[] target, [Out] double[] up);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraGetViewMatrix(IntPtr cam, [Out] double[] view);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraGetProjMatrix(IntPtr cam, [Out] double[] proj);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraSuggestedNearFar(IntPtr cam, ref double near_, ref double far_);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraRenderUpdate(IntPtr cam);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsCameraGetCredits(IntPtr cam);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsCameraGetCreditsShort(IntPtr cam);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsCameraGetCreditsFull(IntPtr cam);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsCameraGetOptions(IntPtr cam);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsCameraGetStatistics(IntPtr cam);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCameraSetOptions(IntPtr cam, [MarshalAs(UnmanagedType.LPStr)] string options);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsDrawsOpaqueGroup(IntPtr cam, ref IntPtr group, ref uint count);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsDrawsTransparentGroup(IntPtr cam, ref IntPtr group, ref uint count);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsDrawsInfographicsGroup(IntPtr cam, ref IntPtr group, ref uint count);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsDrawsCollidersGroup(IntPtr cam, ref IntPtr group, ref uint count);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsDrawsSurfaceTask(IntPtr group, uint index, ref IntPtr mesh, ref IntPtr texColor, ref IntPtr texMask, ref IntPtr baseStruct);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsDrawsSimpleTask(IntPtr group, uint index, ref IntPtr mesh, ref IntPtr texColor, ref IntPtr baseStruct);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsDrawsCamera(IntPtr cam);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsDrawsAtmosphereDensityTexture(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsCelestialName(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern double vtsCelestialMajorRadius(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern double vtsCelestialMinorRadius(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCelestialAtmosphere(IntPtr map, [Out] float[] colors, [Out] double[] parameters);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsCelestialAtmosphereDerivedAttributes(IntPtr map, ref double boundaryThickness, ref double horizontalExponent, ref double verticalExponent);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsFetcherCreateDefault([MarshalAs(UnmanagedType.LPStr)] string createOptions);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsFetcherDestroy(IntPtr fetcher);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern int vtsErrCode();

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsErrMsg();

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsErrCodeToName(int code);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsErrClear();

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void vtsLogCallbackType([MarshalAs(UnmanagedType.LPStr)] string msg);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsLogSetMaskStr([MarshalAs(UnmanagedType.LPStr)] string mask);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsLogSetMaskCode(uint mask);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsLogSetConsole([MarshalAs(UnmanagedType.I1)] bool enable);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsLogSetFile([MarshalAs(UnmanagedType.LPStr)] string filename);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsLogSetThreadName([MarshalAs(UnmanagedType.LPStr)] string name);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsLogAddSink(uint mask, vtsLogCallbackType callback);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsLogClearSinks();

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsLog(uint level, [MarshalAs(UnmanagedType.LPStr)] string message);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsMapCreate([MarshalAs(UnmanagedType.LPStr)] string createOptions, IntPtr fetcher);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMapDestroy(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMapSetCustomData(IntPtr map, IntPtr data);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsMapGetCustomData(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMapSetConfigPaths(IntPtr map, [MarshalAs(UnmanagedType.LPStr)] string mapconfigPath, [MarshalAs(UnmanagedType.LPStr)] string authPath);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsMapGetConfigPath(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
[return: MarshalAs(UnmanagedType.I1)]
public static extern bool vtsMapGetConfigAvailable(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
[return: MarshalAs(UnmanagedType.I1)]
public static extern bool vtsMapGetConfigReady(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
[return: MarshalAs(UnmanagedType.I1)]
public static extern bool vtsMapGetProjected(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
[return: MarshalAs(UnmanagedType.I1)]
public static extern bool vtsMapGetRenderComplete(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern double vtsMapGetRenderProgress(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMapDataInitialize(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMapDataUpdate(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMapDataFinalize(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMapDataAllRun(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMapRenderInitialize(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMapRenderUpdate(IntPtr map, double elapsedTime);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMapRenderFinalize(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsMapGetOptions(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsMapGetStatistics(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMapSetOptions(IntPtr map, [MarshalAs(UnmanagedType.LPStr)] string options);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMapConvert(IntPtr map, [In] double[] pointFrom, [Out] double[] pointTo, uint srsFrom, uint SrsTo);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMathMul44x44([Out] double[] result, [In] double[] l, [In] double[] r);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMathMul33x33([Out] double[] result, [In] double[] l, [In] double[] r);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMathMul44x4([Out] double[] result, [In] double[] l, [In] double[] r);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMathInverse44([Out] double[] result, [In] double[] r);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMathInverse33([Out] double[] result, [In] double[] r);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsNavigationCreate(IntPtr cam);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationDestroy(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationPan(IntPtr nav, [In] double[] value);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationRotate(IntPtr nav, [In] double[] value);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationZoom(IntPtr nav, double value);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationResetAltitude(IntPtr nav);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationResetNavigationMode(IntPtr nav);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationSetSubjective(IntPtr nav, [MarshalAs(UnmanagedType.I1)] bool subjective, [MarshalAs(UnmanagedType.I1)] bool convert);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationSetPoint(IntPtr nav, [In] double[] point);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationSetRotation(IntPtr nav, [In] double[] point);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationSetViewExtent(IntPtr nav, double viewExtent);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationSetFov(IntPtr nav, double fov);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationSetAutoRotation(IntPtr nav, double value);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationSetPositionJson(IntPtr nav, [MarshalAs(UnmanagedType.LPStr)] string position);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationSetPositionUrl(IntPtr nav, [MarshalAs(UnmanagedType.LPStr)] string position);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
[return: MarshalAs(UnmanagedType.I1)]
public static extern bool vtsNavigationGetSubjective(IntPtr nav);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationGetPoint(IntPtr nav, [Out] double[] point);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationGetRotation(IntPtr nav, [Out] double[] rot);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern double vtsNavigationGetViewExtent(IntPtr nav);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern double vtsNavigationGetFov(IntPtr nav);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern double vtsNavigationGetAutoRotation(IntPtr nav);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsNavigationGetPositionUrl(IntPtr nav);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsNavigationGetPositionJson(IntPtr nav);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsNavigationGetOptions(IntPtr nav);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsNavigationSetOptions(IntPtr nav, [MarshalAs(UnmanagedType.LPStr)] string options);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern uint vtsGpuTypeSize(uint type);

[UnmanagedFunctionPointer(CallingConvention.Cdecl)]
public delegate void vtsResourceDeleterCallbackType(IntPtr ptr);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsResourceSetUserData(IntPtr resource, IntPtr data, vtsResourceDeleterCallbackType deleter);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsResourceSetMemoryCost(IntPtr resource, uint ramMem, uint gpuMem);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsResourceGetId(IntPtr resource);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsTextureGetResolution(IntPtr resource, ref uint width, ref uint height, ref uint components);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern uint vtsTextureGetType(IntPtr resource);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern uint vtsTextureGetInternalFormat(IntPtr resource);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern uint vtsTextureGetFilterMode(IntPtr resource);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern uint vtsTextureGetWrapMode(IntPtr resource);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsTextureGetBuffer(IntPtr resource, ref IntPtr data, ref uint size);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern uint vtsMeshGetFaceMode(IntPtr resource);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMeshGetVertices(IntPtr resource, ref IntPtr data, ref uint size, ref uint count);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMeshGetIndices(IntPtr resource, ref IntPtr data, ref uint size, ref uint count);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsMeshGetAttribute(IntPtr resource, uint index, ref uint offset, ref uint stride, ref uint components, ref uint type, [MarshalAs(UnmanagedType.I1)] ref bool enable, [MarshalAs(UnmanagedType.I1)] ref bool normalized);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
[return: MarshalAs(UnmanagedType.I1)]
public static extern bool vtsMapGetSearchable(IntPtr map);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsMapSearch(IntPtr map, [MarshalAs(UnmanagedType.LPStr)] string query);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsMapSearchAt(IntPtr map, [MarshalAs(UnmanagedType.LPStr)] string query, [In] double[] point);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsSearchDestroy(IntPtr search);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
[return: MarshalAs(UnmanagedType.I1)]
public static extern bool vtsSearchGetDone(IntPtr search);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern uint vtsSearchGetResultsCount(IntPtr search);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern IntPtr vtsSearchGetResultData(IntPtr search, uint index);

[DllImport(LibName, CallingConvention = CallingConvention.Cdecl)]
public static extern void vtsSearchUpdateDistances(IntPtr search, [In] double[] point);

	}
}

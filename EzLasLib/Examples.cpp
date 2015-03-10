// Examples.cpp

/**********************************************
Copyright 2015 Esri
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
either express or implied. See the License for the specific
language governing permissions and limitations under the License
**********************************************/

#include "stdafx.h"
#include "EzLasAPIs.h"
//=============================//

void ReportErrorInfo(iEzLasObject* pObject)
{
  if(pObject)
  {
    wchar_t* errInfo = NULL;
    pObject->AccessErrorDescription(&errInfo);
    wprintf(L"Process failed: %s\n", errInfo);
  }
}
//-----------------------------//

HRESULT Test(wchar_t* fileName)
{
  // Create the reader object and obtain iEzLasReader interface. 

  iEzLasReader* pReader = NULL;
  HRESULT hr = EzLasObjectFactory::CreateObject(EzLasOID_Reader, EzLasIID_Reader, (void**)&pReader);

  if(FAILED(hr))
  {
    printf("Failed to create EzLasReader.\n");
    return(hr);
  }

  // Initialize the reader object.

  if(FAILED(hr = pReader->Init(fileName, 100))) // Use 100% CUP power.
  {
    ReportErrorInfo(pReader); // Get the error msg. Do this before calling any other method.
    pReader->Release();
    return(hr);
  }

  // **********************************************************
  // *********************************
  // IMPORTANT: for the sake of easy reading, the rest of the code will skip error
  // checking and reporting. Real code should always check the returnd error code.
  // *********************************
  // **********************************************************

  // File info.

  bool bRearranged = false;
  pReader->GetIsRearranged(bRearranged);

  UINT64 originalSize = 0, compressedSize = 0;
  pReader->GetFileSizes(originalSize, compressedSize);

  // LAS header info.

  EzLasHeader header;
  pReader->GetLasHeader(header);

  // Coordinate system info

  bool bWKT = false;
  bool bEVLRs = false;
  long recordCount = 0;
  iEzLasMemoryBuffer* pMemBuffer = NULL;

  hr = pReader->GetCoordSystemRecords(&pMemBuffer, recordCount, bEVLRs, bWKT);

  if(pMemBuffer) // pMemBuffer will be NULL if there is no coordinate system info.
  {
    long bufferSize = 0;
    char* records = NULL;
    pMemBuffer->AccessBuffer(&records, bufferSize);

    pMemBuffer->Release();
  }

  // Class codes.

  iEzLasLongArray* pCodes = NULL;
  iEzLasLong64Array* pPointCounts = NULL;

  hr = pReader->GetUniqueClassCodes(&pCodes, &pPointCounts);

  pCodes->Release();
  pPointCounts->Release();

  // First 10 point records.

  static const long c_pointCount = 10;

  char* extraBytes = NULL;
  long extraSize = header.pointDataExtraBytesSize;

  if(extraSize)
    extraBytes = new char[extraSize * c_pointCount];

  UINT32 pointDataMask = EzLasPointAll; // Use 'EzLasPointXY | EzLasPointZ' if only xyz are needed. 
  EzLasPointInfo points[c_pointCount];

  UINT64 beginID = 1;
  hr = pReader->QueryPoints(c_pointCount, beginID, pointDataMask, points, extraBytes);

  delete[] extraBytes;

  // 10 points arround the center of the file extent with class code == 2.

  // -- 1. Prepare query filter.

  iEzLasQueryFilter* pFilter = NULL;
  hr = EzLasObjectFactory::CreateObject(EzLasOID_QueryFilter, EzLasIID_QueryFilter, (void**)&pFilter);

  double dx = 0.25 * (header.xMax - header.xMin);
  double dy = 0.25 * (header.yMax - header.yMin);

  double xMin = header.xMin + dx, xMax = xMin + dx;
  double yMin = header.yMin + dy, yMax = yMin + dy;

  EzLasExtent3D aoi;
  aoi.zMin = -DBL_MAX; aoi.zMax = DBL_MAX;
  aoi.xMin = xMin; aoi.yMin = yMin; aoi.xMax = xMax; aoi.yMax = yMax;

  pFilter->SetAOI(&aoi);
  pFilter->AddClassCode(2);

  // -- 2. Get the point enumerator.

  iEzLasEnumPoint* pPoints = NULL;
  pointDataMask = EzLasPointAll;

  hr = pReader->GetPoints(pointDataMask, pFilter, &pPoints);

  // -- 3. Query points.

  long pointCount = 0;
  long arraySize = 10;
  extraBytes = NULL;

  hr = pPoints->QueryNext(arraySize, pointCount, points, extraBytes);

  if(pointCount)
    printf("10 points arround the center of the file extent:\n");
  else
    printf("10 points arround the center of the file extent: None\n");

  // -- 4. Release objects.

  pFilter->Release();
  pPoints->Release();

  // Release the reader object.

  pReader->Release();

  return(hr);
}
//-----------------------------//

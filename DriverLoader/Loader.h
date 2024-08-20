#pragma once
#include <ntifs.h>

BOOLEAN UpdateIAT(CHAR* imageBuffer);

BOOLEAN LoadDriver(PUCHAR fileBuffer);
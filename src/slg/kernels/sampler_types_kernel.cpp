#include <string>
namespace slg { namespace ocl {
std::string KernelSource_sampler_types = 
"#line 2 \"sampler_types.cl\"\n"
"\n"
"/***************************************************************************\n"
" * Copyright 1998-2015 by authors (see AUTHORS.txt)                        *\n"
" *                                                                         *\n"
" *   This file is part of LuxRender.                                       *\n"
" *                                                                         *\n"
" * Licensed under the Apache License, Version 2.0 (the \"License\");         *\n"
" * you may not use this file except in compliance with the License.        *\n"
" * You may obtain a copy of the License at                                 *\n"
" *                                                                         *\n"
" *     http://www.apache.org/licenses/LICENSE-2.0                          *\n"
" *                                                                         *\n"
" * Unless required by applicable law or agreed to in writing, software     *\n"
" * distributed under the License is distributed on an \"AS IS\" BASIS,       *\n"
" * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.*\n"
" * See the License for the specific language governing permissions and     *\n"
" * limitations under the License.                                          *\n"
" ***************************************************************************/\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Indices of Sample related u[] array\n"
"//------------------------------------------------------------------------------\n"
"\n"
"#if defined(SLG_OPENCL_KERNEL)\n"
"\n"
"#define IDX_SCREEN_X 0\n"
"#define IDX_SCREEN_Y 1\n"
"#define IDX_EYE_TIME 2\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"#define IDX_EYE_PASSTHROUGH 3\n"
"#define IDX_DOF_X 4\n"
"#define IDX_DOF_Y 5\n"
"#define IDX_BSDF_OFFSET 6\n"
"#else\n"
"#define IDX_DOF_X 3\n"
"#define IDX_DOF_Y 4\n"
"#define IDX_BSDF_OFFSET 5\n"
"#endif\n"
"\n"
"// Relative to IDX_BSDF_OFFSET + PathDepth * VERTEX_SAMPLE_SIZE\n"
"#if defined(PARAM_HAS_PASSTHROUGH)\n"
"\n"
"#define IDX_PASSTHROUGH 0\n"
"#define IDX_BSDF_X 1\n"
"#define IDX_BSDF_Y 2\n"
"#define IDX_DIRECTLIGHT_X 3\n"
"#define IDX_DIRECTLIGHT_Y 4\n"
"#define IDX_DIRECTLIGHT_Z 5\n"
"#define IDX_DIRECTLIGHT_W 6\n"
"#define IDX_DIRECTLIGHT_A 7\n"
"#define IDX_RR 8\n"
"\n"
"#define VERTEX_SAMPLE_SIZE 9\n"
"\n"
"#else\n"
"\n"
"#define IDX_BSDF_X 0\n"
"#define IDX_BSDF_Y 1\n"
"#define IDX_DIRECTLIGHT_X 2\n"
"#define IDX_DIRECTLIGHT_Y 3\n"
"#define IDX_DIRECTLIGHT_Z 4\n"
"#define IDX_DIRECTLIGHT_W 5\n"
"#define IDX_RR 6\n"
"\n"
"#define VERTEX_SAMPLE_SIZE 7\n"
"#endif\n"
"\n"
"#if (PARAM_SAMPLER_TYPE == 0) || (PARAM_SAMPLER_TYPE == 2)\n"
"#define TOTAL_U_SIZE 2\n"
"#endif\n"
"\n"
"#if (PARAM_SAMPLER_TYPE == 1)\n"
"#define TOTAL_U_SIZE (IDX_BSDF_OFFSET + PARAM_MAX_PATH_DEPTH * VERTEX_SAMPLE_SIZE)\n"
"#endif\n"
"\n"
"#endif\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Sample data types\n"
"//------------------------------------------------------------------------------\n"
"\n"
"// This is defined only under OpenCL because of variable size structures\n"
"#if defined(SLG_OPENCL_KERNEL)\n"
"\n"
"typedef struct {\n"
"	SampleResult result;\n"
"} RandomSample;\n"
"\n"
"typedef struct {\n"
"	SampleResult result;\n"
"\n"
"	float totalI;\n"
"\n"
"	// Using ushort here totally freeze the ATI driver\n"
"	unsigned int largeMutationCount, smallMutationCount;\n"
"	unsigned int current, proposed, consecutiveRejects;\n"
"\n"
"	float weight;\n"
"	SampleResult currentResult;\n"
"} MetropolisSample;\n"
"\n"
"typedef struct {\n"
"	unsigned int pass;\n"
"\n"
"	SampleResult result;\n"
"} SobolSample;\n"
"\n"
"#if (PARAM_SAMPLER_TYPE == 0)\n"
"typedef RandomSample Sample;\n"
"#endif\n"
"\n"
"#if (PARAM_SAMPLER_TYPE == 1)\n"
"typedef MetropolisSample Sample;\n"
"#endif\n"
"\n"
"#if (PARAM_SAMPLER_TYPE == 2)\n"
"typedef SobolSample Sample;\n"
"#endif\n"
"\n"
"#endif\n"
"\n"
"//------------------------------------------------------------------------------\n"
"// Sampler data types\n"
"//------------------------------------------------------------------------------\n"
"\n"
"typedef enum {\n"
"	RANDOM = 0,\n"
"	METROPOLIS = 1,\n"
"	SOBOL = 2\n"
"} SamplerType;\n"
"\n"
"typedef struct {\n"
"	SamplerType type;\n"
"	union {\n"
"		struct {\n"
"			float largeMutationProbability, imageMutationRange;\n"
"			unsigned int maxRejects;\n"
"		} metropolis;\n"
"	};\n"
"} Sampler;\n"
"\n"
"#define SOBOL_BITS 32\n"
"#define SOBOL_MAX_DIMENSIONS 21201\n"
; } }

// SPDX-License-Identifier: Apache-2.0
// ----------------------------------------------------------------------------
// Copyright 2011-2022 Arm Limited
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy
// of the License at:
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
// ----------------------------------------------------------------------------

/**
 * @brief Functions for the library entrypoint.
 */

#include <array>
#include <cstring>
#include <new>

#include "astcenc.h"
#include "astcenc_internal.h"
#include "astcenc_diagnostic_trace.h"

#include "../../../include/mango/core/cpuinfo.hpp"

/**
 * @brief Record of the quality tuning parameter values.
 *
 * See the @c astcenc_config structure for detailed parameter documentation.
 *
 * Note that the mse_overshoot entries are scaling factors relative to the base MSE to hit db_limit.
 * A 20% overshoot is harder to hit for a higher base db_limit, so we may actually use lower ratios
 * for the more through search presets because the underlying db_limit is so much higher.
 */
struct astcenc_preset_config
{
	float quality;
	unsigned int tune_partition_count_limit;
	unsigned int tune_partition_index_limit;
	unsigned int tune_block_mode_limit;
	unsigned int tune_refinement_limit;
	unsigned int tune_candidate_limit;
	float tune_db_limit_a_base;
	float tune_db_limit_b_base;
	float tune_mode0_mse_overshoot;
	float tune_refinement_mse_overshoot;
	float tune_2_partition_early_out_limit_factor;
	float tune_3_partition_early_out_limit_factor;
	float tune_2_plane_early_out_limit_correlation;
	unsigned int tune_low_weight_count_limit;
};


/**
 * @brief The static quality presets that are built-in for high bandwidth
 * presets (x < 25 texels per block).
 */
static const std::array<astcenc_preset_config, 5> preset_configs_high {{
	{
		ASTCENC_PRE_FASTEST,
		2, 10, 43, 2, 2, 85.2f, 63.2f, 3.5f, 3.5f, 1.0f, 1.0f, 0.5f, 25
	}, {
		ASTCENC_PRE_FAST,
		3, 14, 55, 3, 3, 85.2f, 63.2f, 3.5f, 3.5f, 1.0f, 1.1f, 0.65f, 20
	}, {
		ASTCENC_PRE_MEDIUM,
		4, 28, 76, 3, 3, 95.0f, 70.0f, 2.5f, 2.5f, 1.2f, 1.25f, 0.85f, 16
	}, {
		ASTCENC_PRE_THOROUGH,
		4, 76, 93, 4, 4, 105.0f, 77.0f, 10.0f, 10.0f, 2.5f, 1.25f, 0.95f, 12
	}, {
		ASTCENC_PRE_EXHAUSTIVE,
		4, 1024, 100, 4, 4, 200.0f, 200.0f, 10.0f, 10.0f, 10.0f, 10.0f, 0.99f, 0
	}
}};

/**
 * @brief The static quality presets that are built-in for medium bandwidth
 * presets (25 <= x < 64 texels per block).
 */
static const std::array<astcenc_preset_config, 5> preset_configs_mid {{
	{
		ASTCENC_PRE_FASTEST,
		2, 10, 43, 2, 2, 85.2f, 63.2f, 3.5f, 3.5f, 1.0f, 1.0f, 0.5f, 20
	}, {
		ASTCENC_PRE_FAST,
		3, 15, 55, 3, 3, 85.2f, 63.2f, 3.5f, 3.5f, 1.0f, 1.1f, 0.5f, 16
	}, {
		ASTCENC_PRE_MEDIUM,
		4, 30, 76, 3, 3, 95.0f, 70.0f, 3.0f, 3.0f, 1.2f, 1.25f, 0.75f, 14
	}, {
		ASTCENC_PRE_THOROUGH,
		4, 76, 93, 4, 4, 105.0f, 77.0f, 10.0f, 10.0f, 2.5f, 1.25f, 0.95f, 10
	}, {
		ASTCENC_PRE_EXHAUSTIVE,
		4, 1024, 100, 4, 4, 200.0f, 200.0f, 10.0f, 10.0f, 10.0f, 10.0f, 0.99f, 0
	}
}};


/**
 * @brief The static quality presets that are built-in for low bandwidth
 * presets (64 <= x texels per block).
 */
static const std::array<astcenc_preset_config, 5> preset_configs_low {{
	{
		ASTCENC_PRE_FASTEST,
		2, 10, 40, 2, 2, 85.0f, 63.0f, 3.5f, 3.5f, 1.0f, 1.0f, 0.5f, 20
	}, {
		ASTCENC_PRE_FAST,
		2, 15, 55, 3, 3, 85.0f, 63.0f, 3.5f, 3.5f, 1.0f, 1.1f, 0.5f, 16
	}, {
		ASTCENC_PRE_MEDIUM,
		3, 30, 76, 3, 3, 95.0f, 70.0f, 3.5f, 3.5f, 1.2f, 1.25f, 0.65f, 12
	}, {
		ASTCENC_PRE_THOROUGH,
		4, 75, 92, 4, 4, 105.0f, 77.0f, 10.0f, 10.0f, 2.5f, 1.25f, 0.85f, 10
	}, {
		ASTCENC_PRE_EXHAUSTIVE,
		4, 1024, 100, 4, 4, 200.0f, 200.0f, 10.0f, 10.0f, 10.0f, 10.0f, 0.99f, 0
	}
}};

/**
 * @brief Validate CPU floating point meets assumptions made in the codec.
 *
 * The codec is written with the assumption that a float threaded through the @c if32 union will be
 * stored and reloaded as a 32-bit IEEE-754 float with round-to-nearest rounding. This is always the
 * case in an IEEE-754 compliant system, however not every system or compilation mode is actually
 * IEEE-754 compliant. This normally fails if the code is compiled with fast math enabled.
 *
 * @return Return @c ASTCENC_SUCCESS if validated, otherwise an error on failure.
 */
static astcenc_error validate_cpu_float()
{
	if32 p;
	volatile float xprec_testval = 2.51f;
	p.f = xprec_testval + 12582912.0f;
	float q = p.f - 12582912.0f;

	if (q != 3.0f)
	{
		return ASTCENC_ERR_BAD_CPU_FLOAT;
	}

	return ASTCENC_SUCCESS;
}

/**
 * @brief Validate CPU ISA support meets the requirements of this build of the library.
 *
 * Each library build is statically compiled for a particular set of CPU ISA features, such as the
 * SIMD support or other ISA extensions such as POPCNT. This function checks that the host CPU
 * actually supports everything this build needs.
 *
 * @return Return @c ASTCENC_SUCCESS if validated, otherwise an error on failure.
 */
static astcenc_error validate_cpu_isa()
{
	mango::u64 flags = mango::getCPUFlags();
	MANGO_UNREFERENCED(flags);

	#if ASTCENC_SSE >= 41
		if (!(flags & mango::INTEL_SSE4_1))
		{
			return ASTCENC_ERR_BAD_CPU_ISA;
		}
	#endif

	#if ASTCENC_POPCNT >= 1
		if (!(flags & mango::INTEL_POPCNT))
		{
			return ASTCENC_ERR_BAD_CPU_ISA;
		}
	#endif

	#if ASTCENC_F16C >= 1
		if (!(flags & mango::INTEL_F16C))
		{
			return ASTCENC_ERR_BAD_CPU_ISA;
		}
	#endif

	#if ASTCENC_AVX >= 2
		if (!(flags & mango::INTEL_AVX2))
		{
			return ASTCENC_ERR_BAD_CPU_ISA;
		}
	#endif

	return ASTCENC_SUCCESS;
}

/**
 * @brief Validate config profile.
 *
 * @param profile   The profile to check.
 *
 * @return Return @c ASTCENC_SUCCESS if validated, otherwise an error on failure.
 */
static astcenc_error validate_profile(
	astcenc_profile profile
) {
	// Values in this enum are from an external user, so not guaranteed to be
	// bounded to the enum values
	switch (static_cast<int>(profile))
	{
	case ASTCENC_PRF_LDR_SRGB:
	case ASTCENC_PRF_LDR:
	case ASTCENC_PRF_HDR_RGB_LDR_A:
	case ASTCENC_PRF_HDR:
		return ASTCENC_SUCCESS;
	default:
		return ASTCENC_ERR_BAD_PROFILE;
	}
}

/**
 * @brief Validate block size.
 *
 * @param block_x   The block x dimensions.
 * @param block_y   The block y dimensions.
 * @param block_z   The block z dimensions.
 *
 * @return Return @c ASTCENC_SUCCESS if validated, otherwise an error on failure.
 */
static astcenc_error validate_block_size(
	unsigned int block_x,
	unsigned int block_y,
	unsigned int block_z
) {
	// Test if this is a legal block size at all
	bool is_legal = (((block_z <= 1) && is_legal_2d_block_size(block_x, block_y)) ||
	                 ((block_z >= 2) && is_legal_3d_block_size(block_x, block_y, block_z)));
	if (!is_legal)
	{
		return ASTCENC_ERR_BAD_BLOCK_SIZE;
	}

	// Test if this build has sufficient capacity for this block size
	bool have_capacity = (block_x * block_y * block_z) <= BLOCK_MAX_TEXELS;
	if (!have_capacity)
	{
		return ASTCENC_ERR_NOT_IMPLEMENTED;
	}

	return ASTCENC_SUCCESS;
}

/**
 * @brief Validate flags.
 *
 * @param flags   The flags to check.
 *
 * @return Return @c ASTCENC_SUCCESS if validated, otherwise an error on failure.
 */
static astcenc_error validate_flags(
	unsigned int flags
) {
	// Flags field must not contain any unknown flag bits
	unsigned int exMask = ~ASTCENC_ALL_FLAGS;
	if (popcount(flags & exMask) != 0)
	{
		return ASTCENC_ERR_BAD_FLAGS;
	}

	// Flags field must only contain at most a single map type
	exMask = ASTCENC_FLG_MAP_MASK
	       | ASTCENC_FLG_MAP_NORMAL
	       | ASTCENC_FLG_MAP_RGBM;
	if (popcount(flags & exMask) > 1)
	{
		return ASTCENC_ERR_BAD_FLAGS;
	}

	return ASTCENC_SUCCESS;
}

#if !defined(ASTCENC_DECOMPRESS_ONLY)

/**
 * @brief Validate single channel compression swizzle.
 *
 * @param swizzle   The swizzle to check.
 *
 * @return Return @c ASTCENC_SUCCESS if validated, otherwise an error on failure.
 */
static astcenc_error validate_compression_swz(
	astcenc_swz swizzle
) {
	// Not all enum values are handled; SWZ_Z is invalid for compression
	switch (static_cast<int>(swizzle))
	{
	case ASTCENC_SWZ_R:
	case ASTCENC_SWZ_G:
	case ASTCENC_SWZ_B:
	case ASTCENC_SWZ_A:
	case ASTCENC_SWZ_0:
	case ASTCENC_SWZ_1:
		return ASTCENC_SUCCESS;
	default:
		return ASTCENC_ERR_BAD_SWIZZLE;
	}
}

/**
 * @brief Validate overall compression swizzle.
 *
 * @param swizzle   The swizzle to check.
 *
 * @return Return @c ASTCENC_SUCCESS if validated, otherwise an error on failure.
 */
static astcenc_error validate_compression_swizzle(
	const astcenc_swizzle& swizzle
) {
	if (validate_compression_swz(swizzle.r) ||
	    validate_compression_swz(swizzle.g) ||
	    validate_compression_swz(swizzle.b) ||
	    validate_compression_swz(swizzle.a))
	{
		return ASTCENC_ERR_BAD_SWIZZLE;
	}

	return ASTCENC_SUCCESS;
}
#endif

/**
 * Validate that an incoming configuration is in-spec.
 *
 * This function can respond in two ways:
 *
 *   * Numerical inputs that have valid ranges are clamped to those valid ranges. No error is thrown
 *     for out-of-range inputs in this case.
 *   * Numerical inputs and logic inputs are are logically invalid and which make no sense
 *     algorithmically will return an error.
 *
 * @param[in,out] config   The input compressor configuration.
 *
 * @return Return @c ASTCENC_SUCCESS if validated, otherwise an error on failure.
 */
static astcenc_error validate_config(
	astcenc_config &config
) {
	astcenc_error status;

	status = validate_profile(config.profile);
	if (status != ASTCENC_SUCCESS)
	{
		return status;
	}

	status = validate_flags(config.flags);
	if (status != ASTCENC_SUCCESS)
	{
		return status;
	}

	status = validate_block_size(config.block_x, config.block_y, config.block_z);
	if (status != ASTCENC_SUCCESS)
	{
		return status;
	}

#if defined(ASTCENC_DECOMPRESS_ONLY)
	// Decompress-only builds only support decompress-only contexts
	if (!(config.flags & ASTCENC_FLG_DECOMPRESS_ONLY))
	{
		return ASTCENC_ERR_BAD_PARAM;
	}
#endif

	config.rgbm_m_scale = astc::max(config.rgbm_m_scale, 1.0f);

	config.tune_partition_count_limit = astc::clamp(config.tune_partition_count_limit, 1u, 4u);
	config.tune_partition_index_limit = astc::clamp(config.tune_partition_index_limit, 1u, BLOCK_MAX_PARTITIONINGS);
	config.tune_block_mode_limit = astc::clamp(config.tune_block_mode_limit, 1u, 100u);
	config.tune_refinement_limit = astc::max(config.tune_refinement_limit, 1u);
	config.tune_candidate_limit = astc::clamp(config.tune_candidate_limit, 1u, TUNE_MAX_TRIAL_CANDIDATES);
	config.tune_db_limit = astc::max(config.tune_db_limit, 0.0f);
	config.tune_mode0_mse_overshoot = astc::max(config.tune_mode0_mse_overshoot, 1.0f);
	config.tune_refinement_mse_overshoot = astc::max(config.tune_refinement_mse_overshoot, 1.0f);
	config.tune_2_partition_early_out_limit_factor = astc::max(config.tune_2_partition_early_out_limit_factor, 0.0f);
	config.tune_3_partition_early_out_limit_factor = astc::max(config.tune_3_partition_early_out_limit_factor, 0.0f);
	config.tune_2_plane_early_out_limit_correlation = astc::max(config.tune_2_plane_early_out_limit_correlation, 0.0f);

	// Specifying a zero weight color component is not allowed; force to small value
	float max_weight = astc::max(astc::max(config.cw_r_weight, config.cw_g_weight),
	                             astc::max(config.cw_b_weight, config.cw_a_weight));
	if (max_weight > 0.0f)
	{
		max_weight /= 1000.0f;
		config.cw_r_weight = astc::max(config.cw_r_weight, max_weight);
		config.cw_g_weight = astc::max(config.cw_g_weight, max_weight);
		config.cw_b_weight = astc::max(config.cw_b_weight, max_weight);
		config.cw_a_weight = astc::max(config.cw_a_weight, max_weight);
	}
	// If all color components error weights are zero then return an error
	else
	{
		return ASTCENC_ERR_BAD_PARAM;
	}

	return ASTCENC_SUCCESS;
}

/* See header for documentation. */
astcenc_error astcenc_config_init(
	astcenc_profile profile,
	unsigned int block_x,
	unsigned int block_y,
	unsigned int block_z,
	float quality,
	unsigned int flags,
	astcenc_config* configp
) {
	astcenc_error status;
	astcenc_config& config = *configp;

	// Zero init all config fields; although most of will be over written
	std::memset(&config, 0, sizeof(config));

	// Process the block size
	block_z = astc::max(block_z, 1u); // For 2D blocks Z==0 is accepted, but convert to 1
	status = validate_block_size(block_x, block_y, block_z);
	if (status != ASTCENC_SUCCESS)
	{
		return status;
	}

	config.block_x = block_x;
	config.block_y = block_y;
	config.block_z = block_z;

	float texels = static_cast<float>(block_x * block_y * block_z);
	float ltexels = logf(texels) / logf(10.0f);

	// Process the performance quality level or preset; note that this must be done before we
	// process any additional settings, such as color profile and flags, which may replace some of
	// these settings with more use case tuned values
	if (quality < ASTCENC_PRE_FASTEST ||
	    quality > ASTCENC_PRE_EXHAUSTIVE)
	{
		return ASTCENC_ERR_BAD_QUALITY;
	}

	static const std::array<astcenc_preset_config, 5>* preset_configs;
	int texels_int = block_x * block_y * block_z;
	if (texels_int < 25)
	{
		preset_configs = &preset_configs_high;
	}
	else if (texels_int < 64)
	{
		preset_configs = &preset_configs_mid;
	}
	else
	{
		preset_configs = &preset_configs_low;
	}

	// Determine which preset to use, or which pair to interpolate
	size_t start;
	size_t end;
	for (end = 0; end < preset_configs->size(); end++)
	{
		if ((*preset_configs)[end].quality >= quality)
		{
			break;
		}
	}

	start = end == 0 ? 0 : end - 1;

	// Start and end node are the same - so just transfer the values.
	if (start == end)
	{
		config.tune_partition_count_limit = (*preset_configs)[start].tune_partition_count_limit;
		config.tune_partition_index_limit = (*preset_configs)[start].tune_partition_index_limit;
		config.tune_block_mode_limit = (*preset_configs)[start].tune_block_mode_limit;
		config.tune_refinement_limit = (*preset_configs)[start].tune_refinement_limit;
		config.tune_candidate_limit = astc::min((*preset_configs)[start].tune_candidate_limit,
		                                        TUNE_MAX_TRIAL_CANDIDATES);
		config.tune_db_limit = astc::max((*preset_configs)[start].tune_db_limit_a_base - 35 * ltexels,
		                                 (*preset_configs)[start].tune_db_limit_b_base - 19 * ltexels);

		config.tune_mode0_mse_overshoot = (*preset_configs)[start].tune_mode0_mse_overshoot;
		config.tune_refinement_mse_overshoot = (*preset_configs)[start].tune_refinement_mse_overshoot;

		config.tune_2_partition_early_out_limit_factor = (*preset_configs)[start].tune_2_partition_early_out_limit_factor;
		config.tune_3_partition_early_out_limit_factor =(*preset_configs)[start].tune_3_partition_early_out_limit_factor;
		config.tune_2_plane_early_out_limit_correlation = (*preset_configs)[start].tune_2_plane_early_out_limit_correlation;
		config.tune_low_weight_count_limit = (*preset_configs)[start].tune_low_weight_count_limit;
	}
	// Start and end node are not the same - so interpolate between them
	else
	{
		auto& node_a = (*preset_configs)[start];
		auto& node_b = (*preset_configs)[end];

		float wt_range = node_b.quality - node_a.quality;
		assert(wt_range > 0);

		// Compute interpolation factors
		float wt_node_a = (node_b.quality - quality) / wt_range;
		float wt_node_b = (quality - node_a.quality) / wt_range;

		#define LERP(param) ((node_a.param * wt_node_a) + (node_b.param * wt_node_b))
		#define LERPI(param) astc::flt2int_rtn(\
		                         (static_cast<float>(node_a.param) * wt_node_a) + \
		                         (static_cast<float>(node_b.param) * wt_node_b))
		#define LERPUI(param) static_cast<unsigned int>(LERPI(param))

		config.tune_partition_count_limit = LERPI(tune_partition_count_limit);
		config.tune_partition_index_limit = LERPI(tune_partition_index_limit);
		config.tune_block_mode_limit = LERPI(tune_block_mode_limit);
		config.tune_refinement_limit = LERPI(tune_refinement_limit);
		config.tune_candidate_limit = astc::min(LERPUI(tune_candidate_limit),
		                                        TUNE_MAX_TRIAL_CANDIDATES);
		config.tune_db_limit = astc::max(LERP(tune_db_limit_a_base) - 35 * ltexels,
		                                 LERP(tune_db_limit_b_base) - 19 * ltexels);

		config.tune_mode0_mse_overshoot = LERP(tune_mode0_mse_overshoot);
		config.tune_refinement_mse_overshoot = LERP(tune_refinement_mse_overshoot);

		config.tune_2_partition_early_out_limit_factor = LERP(tune_2_partition_early_out_limit_factor);
		config.tune_3_partition_early_out_limit_factor = LERP(tune_3_partition_early_out_limit_factor);
		config.tune_2_plane_early_out_limit_correlation = LERP(tune_2_plane_early_out_limit_correlation);
		config.tune_low_weight_count_limit = LERPI(tune_low_weight_count_limit);
		#undef LERP
		#undef LERPI
		#undef LERPUI
	}

	// Set heuristics to the defaults for each color profile
	config.cw_r_weight = 1.0f;
	config.cw_g_weight = 1.0f;
	config.cw_b_weight = 1.0f;
	config.cw_a_weight = 1.0f;

	config.a_scale_radius = 0;

	config.rgbm_m_scale = 0.0f;

	config.profile = profile;

	// Values in this enum are from an external user, so not guaranteed to be
	// bounded to the enum values
	switch (static_cast<int>(profile))
	{
	case ASTCENC_PRF_LDR:
	case ASTCENC_PRF_LDR_SRGB:
		break;
	case ASTCENC_PRF_HDR_RGB_LDR_A:
	case ASTCENC_PRF_HDR:
		config.tune_db_limit = 999.0f;
		break;
	default:
		return ASTCENC_ERR_BAD_PROFILE;
	}

	// Flags field must not contain any unknown flag bits
	status = validate_flags(flags);
	if (status != ASTCENC_SUCCESS)
	{
		return status;
	}

	if (flags & ASTCENC_FLG_MAP_NORMAL)
	{
		// Normal map encoding uses L+A blocks, so allow one more partitioning
		// than normal. We need need fewer bits for endpoints, so more likely
		// to be able to use more partitions than an RGB/RGBA block
		config.tune_partition_count_limit = astc::min(config.tune_partition_count_limit + 1u, 4u);

		config.cw_g_weight = 0.0f;
		config.cw_b_weight = 0.0f;
		config.tune_2_partition_early_out_limit_factor *= 1.5f;
		config.tune_3_partition_early_out_limit_factor *= 1.5f;
		config.tune_2_plane_early_out_limit_correlation = 0.99f;

		// Normals are prone to blocking artifacts on smooth curves
		// so force compressor to try harder here ...
		config.tune_db_limit *= 1.03f;
	}
	else if (flags & ASTCENC_FLG_MAP_MASK)
	{
		// Masks are prone to blocking artifacts on mask edges
		// so force compressor to try harder here ...
		config.tune_db_limit *= 1.03f;
	}
	else if (flags & ASTCENC_FLG_MAP_RGBM)
	{
		config.rgbm_m_scale = 5.0f;
		config.cw_a_weight = 2.0f * config.rgbm_m_scale;
	}
	else // (This is color data)
	{
		// This is a very basic perceptual metric for RGB color data, which weights error
		// significance by the perceptual luminance contribution of each color channel. For
		// luminance the usual weights to compute luminance from a linear RGB value are as
		// follows:
		//
		//     l = r * 0.3 + g * 0.59 + b * 0.11
		//
		// ... but we scale these up to keep a better balance between color and alpha. Note
		// that if the content is using alpha we'd recommend using the -a option to weight
		// the color contribution by the alpha transparency.
		if (flags & ASTCENC_FLG_USE_PERCEPTUAL)
		{
			config.cw_r_weight = 0.30f * 2.25f;
			config.cw_g_weight = 0.59f * 2.25f;
			config.cw_b_weight = 0.11f * 2.25f;
		}
	}
	config.flags = flags;

	return ASTCENC_SUCCESS;
}

/* See header for documentation. */
astcenc_error astcenc_context_alloc(
	const astcenc_config* configp,
	astcenc_context& context
) {
	astcenc_error status;
	const astcenc_config& config = *configp;

	status = validate_cpu_float();
	if (status != ASTCENC_SUCCESS)
	{
		return status;
	}

	status = validate_cpu_isa();
	if (status != ASTCENC_SUCCESS)
	{
		return status;
	}

	context.config = config;

	// Copy the config first and validate the copy (we may modify it)
	status = validate_config(context.config);
	if (status != ASTCENC_SUCCESS)
	{
		return status;
	}

	context.bsd = new block_size_descriptor();
	bool can_omit_modes = static_cast<bool>(config.flags & ASTCENC_FLG_SELF_DECOMPRESS_ONLY);
	init_block_size_descriptor(config.block_x, config.block_y, config.block_z,
	                           can_omit_modes,
	                           config.tune_partition_count_limit,
	                           static_cast<float>(config.tune_block_mode_limit) / 100.0f,
	                           *context.bsd);

#if !defined(ASTCENC_DECOMPRESS_ONLY)
	// Do setup only needed by compression
	if (!(status & ASTCENC_FLG_DECOMPRESS_ONLY))
	{
		// Turn a dB limit into a per-texel error for faster use later
		if ((context.config.profile == ASTCENC_PRF_LDR) || (context.config.profile == ASTCENC_PRF_LDR_SRGB))
		{
			context.config.tune_db_limit = astc::pow(0.1f, context.config.tune_db_limit * 0.1f) * 65535.0f * 65535.0f;
		}
		else
		{
			context.config.tune_db_limit = 0.0f;
		}
	}
#endif

#if defined(ASTCENC_DIAGNOSTICS)
	context->trace_log = new TraceLog(context->config.trace_file_path);
	if (!context->trace_log->m_file)
	{
		return ASTCENC_ERR_DTRACE_FAILURE;
	}

	trace_add_data("block_x", config.block_x);
	trace_add_data("block_y", config.block_y);
	trace_add_data("block_z", config.block_z);
#endif

#if !defined(ASTCENC_DECOMPRESS_ONLY)
	prepare_angular_tables();
#endif

	return ASTCENC_SUCCESS;
}

/* See header dor documentation. */
void astcenc_context_free(
	astcenc_context& context
) {
	delete context.bsd;
#if defined(ASTCENC_DIAGNOSTICS)
	delete context.trace_log;
#endif
}

#if !defined(ASTCENC_DECOMPRESS_ONLY)

/**
 * @brief Compress an image, after any preflight has completed.
 *
 * @param[out] ctxo           The compressor context.
 * @param      image          The intput image.
 * @param      swizzle        The input swizzle.
 * @param[out] buffer         The output array for the compressed data.
 */
static void compress_image(
	astcenc_context& context,
	const astcenc_image& image,
	const astcenc_swizzle& swizzle,
	uint8_t* buffer
) {
	const block_size_descriptor& bsd = *context.bsd;
	astcenc_profile decode_mode = context.config.profile;

	image_block blk;

	int block_x = bsd.xdim;
	int block_y = bsd.ydim;
	blk.texel_count = static_cast<uint8_t>(block_x * block_y);

	int dim_x = image.dim_x;
	int dim_y = image.dim_y;

	int xblocks = (dim_x + block_x - 1) / block_x;
	int yblocks = (dim_y + block_y - 1) / block_y;

	// Populate the block channel weights
	blk.channel_weight = vfloat4(context.config.cw_r_weight,
	                             context.config.cw_g_weight,
	                             context.config.cw_b_weight,
	                             context.config.cw_a_weight);

	// Use preallocated scratch buffer
	auto& temp_buffers = context.working_buffer;

	// Determine if we can use an optimized load function
	bool needs_swz = (swizzle.r != ASTCENC_SWZ_R) || (swizzle.g != ASTCENC_SWZ_G) ||
	                 (swizzle.b != ASTCENC_SWZ_B) || (swizzle.a != ASTCENC_SWZ_A);

	bool needs_hdr = (decode_mode == ASTCENC_PRF_HDR) ||
	                 (decode_mode == ASTCENC_PRF_HDR_RGB_LDR_A);

	bool use_fast_load = !needs_swz && !needs_hdr && image.data_type == ASTCENC_TYPE_U8;

	auto load_func = load_image_block;
	if (use_fast_load)
	{
		load_func = load_image_block_fast_ldr;
	}

	for (int y = 0; y < yblocks; ++y)
	{
		unsigned int yoffset = y * block_y;
		unsigned int xoffset = 0;

		for (int x = 0; x < xblocks; ++x)
		{
			// Test if we can apply some basic alpha-scale RDO
			bool use_full_block = true;
			if (context.config.a_scale_radius != 0)
			{
				int start_x = xoffset;
				int end_x = astc::min(dim_x, start_x + block_x);

				int start_y = yoffset;
				int end_y = astc::min(dim_y, start_y + block_y);

				// SATs accumulate error, so don't test exactly zero. Test for
				// less than 1 alpha in the expanded block footprint that
				// includes the alpha radius.
				int x_footprint = block_x + 2 * (context.config.a_scale_radius - 1);

				int y_footprint = block_y + 2 * (context.config.a_scale_radius - 1);

				float footprint = static_cast<float>(x_footprint * y_footprint);
				float threshold = 0.9f / (255.0f * footprint);

				// Do we have any alpha values?
				use_full_block = false;
				for (int ay = start_y; ay < end_y; ay++)
				{
					for (int ax = start_x; ax < end_x; ax++)
					{
						float a_avg = context.input_alpha_averages[ay * dim_x + ax];
						if (a_avg > threshold)
						{
							use_full_block = true;
							ax = end_x;
							ay = end_y;
						}
					}
				}
			}

			if (use_full_block)
			{
				// Fetch the full block for compression
				load_func(decode_mode, image, blk, bsd, xoffset, yoffset, 0, swizzle);

				// Scale RGB error contribution by the maximum alpha in the block
				// This encourages preserving alpha accuracy in regions with high
				// transparency, and can buy up to 0.5 dB PSNR.
				if (context.config.flags & ASTCENC_FLG_USE_ALPHA_WEIGHT)
				{
					float alpha_scale = blk.data_max.lane<3>() * (1.0f / 65535.0f);
					blk.channel_weight = vfloat4(context.config.cw_r_weight * alpha_scale,
												 context.config.cw_g_weight * alpha_scale,
												 context.config.cw_b_weight * alpha_scale,
												 context.config.cw_a_weight);
				}
			}
			else
			{
				// Apply alpha scale RDO - substitute constant color block
				blk.origin_texel = vfloat4::zero();
				blk.data_min = vfloat4::zero();
				blk.data_mean = vfloat4::zero();
				blk.data_max = vfloat4::zero();
				blk.grayscale = true;
			}

			uint8_t *bp = buffer;
			physical_compressed_block* pcb = reinterpret_cast<physical_compressed_block*>(bp);
			compress_block(context, blk, *pcb, temp_buffers);

			xoffset += block_x;
			buffer += 16;
		}
	}
}

/**
 * @brief Compute regional averages in an image.
 *
 * This function can be called by multiple threads, but only after a single
 * thread calls the setup function @c init_compute_averages().
 *
 * Results are written back into @c img->input_alpha_averages.
 *
 * @param[out] ctx   The context.
 * @param      ag    The average and variance arguments created during setup.
 */
static void compute_averages(
	astcenc_context& context,
	const avg_args &ag,
	unsigned int count
) {
	pixel_region_args arg = ag.arg;
	arg.work_memory = new vfloat4[ag.work_memory_size];

	int size_x = ag.img_size_x;
	int size_y = ag.img_size_y;
	int size_z = ag.img_size_z;

	int step_xy = ag.blk_size_xy;
	int step_z = ag.blk_size_z;

	int y_tasks = (size_y + step_xy - 1) / step_xy;

	for (unsigned int i = 0; i < count; i++)
	{
		int z = (i / (y_tasks)) * step_z;
		int y = (i - (z * y_tasks)) * step_xy;

		arg.size_z = astc::min(step_z, size_z - z);
		arg.offset_z = z;

		arg.size_y = astc::min(step_xy, size_y - y);
		arg.offset_y = y;

		for (int x = 0; x < size_x; x += step_xy)
		{
			arg.size_x = astc::min(step_xy, size_x - x);
			arg.offset_x = x;
			compute_pixel_region_variance(context, arg);
		}
	}

	delete[] arg.work_memory;
}

#endif

/* See header for documentation. */
astcenc_error astcenc_compress_image(
	astcenc_context& context,
	astcenc_image& image,
	const astcenc_swizzle* swizzle,
	uint8_t* data_out
) {
#if defined(ASTCENC_DECOMPRESS_ONLY)
	(void)context;
	(void)imagep;
	(void)swizzle;
	(void)data_out;
	(void)thread_index;
	return ASTCENC_ERR_BAD_CONTEXT;
#else
	astcenc_error status;

	if (context.config.flags & ASTCENC_FLG_DECOMPRESS_ONLY)
	{
		return ASTCENC_ERR_BAD_CONTEXT;
	}

	status = validate_compression_swizzle(*swizzle);
	if (status != ASTCENC_SUCCESS)
	{
		return status;
	}

	if (context.config.a_scale_radius != 0)
	{
		unsigned int count = init_compute_averages(
			image, context.config.a_scale_radius, *swizzle,
			context.avg_preprocess_args);

		compute_averages(context, context.avg_preprocess_args, count);
	}

	compress_image(context, image, *swizzle, data_out);

	return ASTCENC_SUCCESS;
#endif
}

/* See header for documentation. */
astcenc_error astcenc_get_block_info(
	astcenc_context* context,
	const uint8_t data[16],
	astcenc_block_info* info
) {
#if defined(ASTCENC_DECOMPRESS_ONLY)
	(void)context;
	(void)data;
	(void)info;
	return ASTCENC_ERR_BAD_CONTEXT;
#else
	// Decode the compressed data into a symbolic form
	const physical_compressed_block&pcb = *reinterpret_cast<const physical_compressed_block*>(data);
	symbolic_compressed_block scb;
	physical_to_symbolic(*context->bsd, pcb, scb);

	// Fetch the appropriate partition and decimation tables
	block_size_descriptor& bsd = *context->bsd;

	// Start from a clean slate
	memset(info, 0, sizeof(*info));

	// Basic info we can always populate
	info->profile = context->config.profile;

	info->block_x = context->config.block_x;
	info->block_y = context->config.block_y;
	info->block_z = context->config.block_z;
	info->texel_count = bsd.texel_count;

	// Check for error blocks first
	info->is_error_block = scb.block_type == SYM_BTYPE_ERROR;
	if (info->is_error_block)
	{
		return ASTCENC_SUCCESS;
	}

	// Check for constant color blocks second
	info->is_constant_block = scb.block_type == SYM_BTYPE_CONST_F16 ||
	                          scb.block_type == SYM_BTYPE_CONST_U16;
	if (info->is_constant_block)
	{
		return ASTCENC_SUCCESS;
	}

	// Otherwise handle a full block ; known to be valid after conditions above have been checked
	int partition_count = scb.partition_count;
	const auto& pi = bsd.get_partition_info(partition_count, scb.partition_index);

	const block_mode& bm = bsd.get_block_mode(scb.block_mode);
	const decimation_info& di = bsd.get_decimation_info(bm.decimation_mode);

	info->weight_x = di.weight_x;
	info->weight_y = di.weight_y;
	info->weight_z = di.weight_z;

	info->is_dual_plane_block = bm.is_dual_plane != 0;

	info->partition_count = scb.partition_count;
	info->partition_index = scb.partition_index;
	info->dual_plane_component = scb.plane2_component;

	info->color_level_count = get_quant_level(scb.get_color_quant_mode());
	info->weight_level_count = get_quant_level(bm.get_weight_quant_mode());

	// Unpack color endpoints for each active partition
	for (unsigned int i = 0; i < scb.partition_count; i++)
	{
		bool rgb_hdr;
		bool a_hdr;
		vint4 endpnt[2];

		unpack_color_endpoints(context->config.profile,
		                       scb.color_formats[i],
		                       scb.get_color_quant_mode(),
		                       scb.color_values[i],
		                       rgb_hdr, a_hdr,
		                       endpnt[0], endpnt[1]);

		// Store the color endpoint mode info
		info->color_endpoint_modes[i] = scb.color_formats[i];
		info->is_hdr_block = info->is_hdr_block || rgb_hdr || a_hdr;

		// Store the unpacked and decoded color endpoint
		vmask4 hdr_mask(rgb_hdr, rgb_hdr, rgb_hdr, a_hdr);
		for (int j = 0; j < 2; j++)
		{
			vint4 color_lns = lns_to_sf16(endpnt[j]);
			vint4 color_unorm = unorm16_to_sf16(endpnt[j]);
			vint4 datai = select(color_unorm, color_lns, hdr_mask);
			store(float16_to_float(datai), info->color_endpoints[i][j]);
		}
	}

	// Unpack weights for each texel
	int weight_plane1[BLOCK_MAX_TEXELS];
	int weight_plane2[BLOCK_MAX_TEXELS];

	unpack_weights(bsd, scb, di, bm.is_dual_plane, weight_plane1, weight_plane2);
	for (unsigned int i = 0; i < bsd.texel_count; i++)
	{
		info->weight_values_plane1[i] = static_cast<float>(weight_plane1[i]) * (1.0f / WEIGHTS_TEXEL_SUM);
		if (info->is_dual_plane_block)
		{
			info->weight_values_plane2[i] = static_cast<float>(weight_plane2[i]) * (1.0f / WEIGHTS_TEXEL_SUM);
		}
	}

	// Unpack partition assignments for each texel
	for (unsigned int i = 0; i < bsd.texel_count; i++)
	{
		info->partition_assignment[i] = pi.partition_of_texel[i];
	}

	return ASTCENC_SUCCESS;
#endif
}

/* See header for documentation. */
const char* astcenc_get_error_string(
	astcenc_error status
) {
	// Values in this enum are from an external user, so not guaranteed to be
	// bounded to the enum values
	switch (static_cast<int>(status))
	{
	case ASTCENC_SUCCESS:
		return "ASTCENC_SUCCESS";
	case ASTCENC_ERR_OUT_OF_MEM:
		return "ASTCENC_ERR_OUT_OF_MEM";
	case ASTCENC_ERR_BAD_CPU_FLOAT:
		return "ASTCENC_ERR_BAD_CPU_FLOAT";
	case ASTCENC_ERR_BAD_CPU_ISA:
		return "ASTCENC_ERR_BAD_CPU_ISA";
	case ASTCENC_ERR_BAD_PARAM:
		return "ASTCENC_ERR_BAD_PARAM";
	case ASTCENC_ERR_BAD_BLOCK_SIZE:
		return "ASTCENC_ERR_BAD_BLOCK_SIZE";
	case ASTCENC_ERR_BAD_PROFILE:
		return "ASTCENC_ERR_BAD_PROFILE";
	case ASTCENC_ERR_BAD_QUALITY:
		return "ASTCENC_ERR_BAD_QUALITY";
	case ASTCENC_ERR_BAD_FLAGS:
		return "ASTCENC_ERR_BAD_FLAGS";
	case ASTCENC_ERR_BAD_SWIZZLE:
		return "ASTCENC_ERR_BAD_SWIZZLE";
	case ASTCENC_ERR_BAD_CONTEXT:
		return "ASTCENC_ERR_BAD_CONTEXT";
	case ASTCENC_ERR_NOT_IMPLEMENTED:
		return "ASTCENC_ERR_NOT_IMPLEMENTED";
#if defined(ASTCENC_DIAGNOSTICS)
	case ASTCENC_ERR_DTRACE_FAILURE:
		return "ASTCENC_ERR_DTRACE_FAILURE";
#endif
	default:
		return nullptr;
	}
}

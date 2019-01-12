#include "render_target.h"

namespace nimble
{
	static uint32_t g_last_rt_id = 0;

	RenderTarget::RenderTarget() : id(g_last_rt_id++)
	{

	}

	bool RenderTarget::is_scaled()
	{
		return scale_w > 0.0f && scale_h > 0.0f;
	}
} // nimble
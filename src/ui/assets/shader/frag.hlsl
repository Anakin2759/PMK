#define UI_STAGE_PIXEL 1
#include "common.hlsl"

// =========================================================================
// Pixel Shader (Premultiplied Alpha, Correct Shadow Compositing)
// =========================================================================

// 圆角矩形 SDF
float sdRoundedBox(float2 p, float2 b, float4 r)
{
    float2 q_radius = (p.x > 0.0) ? r.yz : r.xw;
    float actual_r = (p.y > 0.0) ? q_radius.x : q_radius.y;

    float2 q = abs(p) - b + actual_r;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - actual_r;
}

float4 main_ps(PSInput input) : SV_Target
{
    // ------------------------------------------------------------
    // 1. 像素坐标（以矩形中心为原点）
    // ------------------------------------------------------------
    float2 p = (input.texcoord - 0.5) * rect_size;
    float2 half_size = rect_size * 0.5;

    // ------------------------------------------------------------
    // 2. 主体 SDF
    // ------------------------------------------------------------
    float dist = sdRoundedBox(p, half_size, radius);

    float edge = fwidth(dist);
    float body_mask = 1.0 - smoothstep(-edge, edge, dist);

    // ------------------------------------------------------------
    // 3. 阴影 SDF（只影响主体外部）
    // ------------------------------------------------------------
    float shadow_alpha = 0.0;

    if (shadow_soft > 0.0)
    {
        float2 shadow_p = p - float2(shadow_offset_x, shadow_offset_y);
        float dist_shadow = sdRoundedBox(shadow_p, half_size, radius);

        shadow_alpha =
            1.0 - smoothstep(-shadow_soft, shadow_soft, dist_shadow);

        // 阴影只存在于主体外部
        shadow_alpha *= (1.0 - body_mask);

        // 强度控制（UI 阴影不宜过重）
        shadow_alpha *= 0.5;
    }

    // ------------------------------------------------------------
    // 4. 主体颜色（预乘 Alpha）
    // ------------------------------------------------------------
    float4 tex = u_texture.Sample(u_sampler, input.texcoord);
    float4 color = tex * input.color;

    // 主体 alpha
    float body_alpha = color.a * body_mask;

    // 预乘
    float3 body_rgb = color.rgb * body_alpha;

    // ------------------------------------------------------------
    // 5. 阴影颜色（预乘 Alpha，纯黑）
    // ------------------------------------------------------------
    float shadow_a = shadow_alpha;
    float3 shadow_rgb = float3(0.0, 0.0, 0.0); // 已预乘

    // ------------------------------------------------------------
    // 6. 合成（预乘 Alpha 空间）
    // ------------------------------------------------------------
    float out_alpha = body_alpha + shadow_a;
    float3 out_rgb = body_rgb + shadow_rgb;

    // 全局透明度（UI 树 Alpha）
    out_alpha *= opacity;
    out_rgb *= opacity;

    // 剔除无效像素
    if (out_alpha < 0.001)
        discard;

    return float4(out_rgb, out_alpha);
}

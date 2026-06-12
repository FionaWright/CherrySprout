#ifndef H_GUI_STYLES_H
#define H_GUI_STYLES_H

#include "imgui.h"

// All generated with GPT-3.5

inline void ApplyCherryBlossomTheme()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 8.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;

    ImVec4* colors = style.Colors;

    // Text
    colors[ImGuiCol_Text]                 = ImVec4(0.95f, 0.93f, 0.88f, 1.00f);
    colors[ImGuiCol_TextDisabled]         = ImVec4(0.55f, 0.60f, 0.55f, 1.00f);

    // Backgrounds
    colors[ImGuiCol_WindowBg]             = ImVec4(0.08f, 0.12f, 0.10f, 1.00f);
    colors[ImGuiCol_ChildBg]              = ImVec4(0.10f, 0.15f, 0.12f, 1.00f);
    colors[ImGuiCol_PopupBg]              = ImVec4(0.11f, 0.16f, 0.13f, 0.98f);

    // Borders
    colors[ImGuiCol_Border]               = ImVec4(0.28f, 0.40f, 0.32f, 0.60f);
    colors[ImGuiCol_BorderShadow]         = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Frames
    colors[ImGuiCol_FrameBg]              = ImVec4(0.16f, 0.25f, 0.19f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.24f, 0.38f, 0.29f, 1.00f);
    colors[ImGuiCol_FrameBgActive]        = ImVec4(0.30f, 0.48f, 0.36f, 1.00f);

    // Titles
    colors[ImGuiCol_TitleBg]              = ImVec4(0.12f, 0.18f, 0.14f, 1.00f);
    colors[ImGuiCol_TitleBgActive]        = ImVec4(0.16f, 0.24f, 0.18f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed]     = ImVec4(0.10f, 0.15f, 0.12f, 1.00f);

    // Buttons (leaf green)
    colors[ImGuiCol_Button]               = ImVec4(0.29f, 0.52f, 0.38f, 1.00f);
    colors[ImGuiCol_ButtonHovered]        = ImVec4(0.38f, 0.65f, 0.47f, 1.00f);
    colors[ImGuiCol_ButtonActive]         = ImVec4(0.24f, 0.43f, 0.31f, 1.00f);

    // Headers
    colors[ImGuiCol_Header]               = ImVec4(0.22f, 0.40f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderHovered]        = ImVec4(0.32f, 0.55f, 0.42f, 1.00f);
    colors[ImGuiCol_HeaderActive]         = ImVec4(0.38f, 0.65f, 0.48f, 1.00f);

    // Tabs (cherry blossom accent)
    colors[ImGuiCol_Tab]                  = ImVec4(0.55f, 0.34f, 0.42f, 1.00f);
    colors[ImGuiCol_TabHovered]           = ImVec4(0.77f, 0.53f, 0.63f, 1.00f);
    colors[ImGuiCol_TabSelected]          = ImVec4(0.68f, 0.46f, 0.56f, 1.00f);
    colors[ImGuiCol_TabDimmed]            = ImVec4(0.30f, 0.22f, 0.25f, 1.00f);
    colors[ImGuiCol_TabDimmedSelected]    = ImVec4(0.48f, 0.32f, 0.39f, 1.00f);

    // Checkboxes / Radio buttons
    colors[ImGuiCol_CheckMark]            = ImVec4(0.95f, 0.72f, 0.80f, 1.00f);

    // Sliders
    colors[ImGuiCol_SliderGrab]           = ImVec4(0.82f, 0.61f, 0.70f, 1.00f);
    colors[ImGuiCol_SliderGrabActive]     = ImVec4(0.95f, 0.72f, 0.80f, 1.00f);

    // Resize grip
    colors[ImGuiCol_ResizeGrip]           = ImVec4(0.62f, 0.45f, 0.52f, 0.50f);
    colors[ImGuiCol_ResizeGripHovered]    = ImVec4(0.82f, 0.61f, 0.70f, 0.80f);
    colors[ImGuiCol_ResizeGripActive]     = ImVec4(0.95f, 0.72f, 0.80f, 1.00f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.08f, 0.12f, 0.10f, 1.00f);
    colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.30f, 0.48f, 0.36f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.62f, 0.48f, 1.00f);
    colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.50f, 0.75f, 0.58f, 1.00f);

    // Separators
    colors[ImGuiCol_Separator]            = ImVec4(0.32f, 0.48f, 0.38f, 0.60f);
    colors[ImGuiCol_SeparatorHovered]     = ImVec4(0.95f, 0.72f, 0.80f, 1.00f);
    colors[ImGuiCol_SeparatorActive]      = ImVec4(1.00f, 0.80f, 0.87f, 1.00f);
}

inline void ApplyCherryTheme()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;

    ImVec4* c = style.Colors;

    // Text
    c[ImGuiCol_Text]         = ImVec4(0.96f, 0.92f, 0.92f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.65f, 0.55f, 0.58f, 1.00f);

    // Backgrounds
    c[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.04f, 0.05f, 1.00f);
    c[ImGuiCol_ChildBg]  = ImVec4(0.11f, 0.05f, 0.06f, 1.00f);
    c[ImGuiCol_PopupBg]  = ImVec4(0.13f, 0.05f, 0.07f, 0.98f);

    // Borders
    c[ImGuiCol_Border] = ImVec4(0.42f, 0.18f, 0.22f, 0.60f);

    // Frame backgrounds
    c[ImGuiCol_FrameBg]        = ImVec4(0.22f, 0.08f, 0.10f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.36f, 0.12f, 0.15f, 1.00f);
    c[ImGuiCol_FrameBgActive]  = ImVec4(0.50f, 0.16f, 0.20f, 1.00f);

    // Window titles
    c[ImGuiCol_TitleBg]          = ImVec4(0.14f, 0.05f, 0.07f, 1.00f);
    c[ImGuiCol_TitleBgActive]    = ImVec4(0.28f, 0.08f, 0.11f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.04f, 0.05f, 1.00f);

    // Buttons
    c[ImGuiCol_Button]        = ImVec4(0.58f, 0.12f, 0.18f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.74f, 0.18f, 0.24f, 1.00f);
    c[ImGuiCol_ButtonActive]  = ImVec4(0.88f, 0.24f, 0.31f, 1.00f);

    // Headers
    c[ImGuiCol_Header]        = ImVec4(0.45f, 0.11f, 0.16f, 1.00f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.62f, 0.16f, 0.22f, 1.00f);
    c[ImGuiCol_HeaderActive]  = ImVec4(0.78f, 0.20f, 0.28f, 1.00f);

    // Tabs
    c[ImGuiCol_Tab]                = ImVec4(0.32f, 0.09f, 0.12f, 1.00f);
    c[ImGuiCol_TabHovered]         = ImVec4(0.71f, 0.18f, 0.25f, 1.00f);
    c[ImGuiCol_TabSelected]        = ImVec4(0.58f, 0.14f, 0.21f, 1.00f);
    c[ImGuiCol_TabDimmed]          = ImVec4(0.18f, 0.06f, 0.08f, 1.00f);
    c[ImGuiCol_TabDimmedSelected]  = ImVec4(0.42f, 0.11f, 0.15f, 1.00f);

    // Checkboxes & radio buttons
    c[ImGuiCol_CheckMark] = ImVec4(1.00f, 0.65f, 0.72f, 1.00f);

    // Sliders
    c[ImGuiCol_SliderGrab]       = ImVec4(0.86f, 0.28f, 0.36f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(1.00f, 0.45f, 0.52f, 1.00f);

    // Scrollbar
    c[ImGuiCol_ScrollbarBg]          = ImVec4(0.08f, 0.03f, 0.04f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.42f, 0.10f, 0.15f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.62f, 0.15f, 0.22f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.82f, 0.22f, 0.30f, 1.00f);

    // Separators
    c[ImGuiCol_Separator]        = ImVec4(0.40f, 0.15f, 0.18f, 0.60f);
    c[ImGuiCol_SeparatorHovered] = ImVec4(0.85f, 0.30f, 0.38f, 1.00f);
    c[ImGuiCol_SeparatorActive]  = ImVec4(1.00f, 0.45f, 0.55f, 1.00f);

    // Accent colors
    c[ImGuiCol_ResizeGrip]        = ImVec4(0.70f, 0.18f, 0.25f, 0.50f);
    c[ImGuiCol_ResizeGripHovered] = ImVec4(0.90f, 0.28f, 0.36f, 0.80f);
    c[ImGuiCol_ResizeGripActive]  = ImVec4(1.00f, 0.40f, 0.50f, 1.00f);
}

inline void ApplyCherryOrchardTheme()
{
    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;

    ImVec4* c = style.Colors;

    // ------------------------------------------------------------------------
    // Text
    // ------------------------------------------------------------------------

    c[ImGuiCol_Text]         = ImVec4(0.95f, 0.93f, 0.90f, 1.00f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.60f, 0.58f, 0.55f, 1.00f);

    // ------------------------------------------------------------------------
    // Backgrounds (dark cherry wine)
    // ------------------------------------------------------------------------

    c[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.06f, 0.07f, 1.00f);
    c[ImGuiCol_ChildBg]  = ImVec4(0.12f, 0.07f, 0.08f, 1.00f);
    c[ImGuiCol_PopupBg]  = ImVec4(0.14f, 0.08f, 0.09f, 0.98f);

    // ------------------------------------------------------------------------
    // Borders (stem green)
    // ------------------------------------------------------------------------

    c[ImGuiCol_Border]       = ImVec4(0.34f, 0.45f, 0.31f, 0.65f);
    c[ImGuiCol_Separator]    = ImVec4(0.34f, 0.45f, 0.31f, 0.50f);
    c[ImGuiCol_SeparatorHovered] = ImVec4(0.52f, 0.67f, 0.44f, 1.00f);
    c[ImGuiCol_SeparatorActive]  = ImVec4(0.60f, 0.78f, 0.50f, 1.00f);

    // ------------------------------------------------------------------------
    // Frames
    // ------------------------------------------------------------------------

    c[ImGuiCol_FrameBg]        = ImVec4(0.20f, 0.11f, 0.13f, 1.00f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.30f, 0.16f, 0.19f, 1.00f);
    c[ImGuiCol_FrameBgActive]  = ImVec4(0.40f, 0.21f, 0.25f, 1.00f);

    // ------------------------------------------------------------------------
    // Titles
    // ------------------------------------------------------------------------

    c[ImGuiCol_TitleBg]          = ImVec4(0.14f, 0.08f, 0.09f, 1.00f);
    c[ImGuiCol_TitleBgActive]    = ImVec4(0.28f, 0.13f, 0.16f, 1.00f);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.06f, 0.07f, 1.00f);

    // ------------------------------------------------------------------------
    // Cherry red buttons
    // ------------------------------------------------------------------------

    c[ImGuiCol_Button]        = ImVec4(0.76f, 0.23f, 0.33f, 1.00f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.88f, 0.32f, 0.43f, 1.00f);
    c[ImGuiCol_ButtonActive]  = ImVec4(0.60f, 0.18f, 0.27f, 1.00f);

    // ------------------------------------------------------------------------
    // Headers
    // ------------------------------------------------------------------------

    c[ImGuiCol_Header]        = ImVec4(0.46f, 0.18f, 0.24f, 1.00f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.63f, 0.24f, 0.33f, 1.00f);
    c[ImGuiCol_HeaderActive]  = ImVec4(0.78f, 0.30f, 0.41f, 1.00f);

    // ------------------------------------------------------------------------
    // Tabs
    // ------------------------------------------------------------------------

    c[ImGuiCol_Tab]               = ImVec4(0.30f, 0.12f, 0.16f, 1.00f);
    c[ImGuiCol_TabHovered]        = ImVec4(0.74f, 0.25f, 0.35f, 1.00f);
    c[ImGuiCol_TabSelected]       = ImVec4(0.58f, 0.20f, 0.28f, 1.00f);
    c[ImGuiCol_TabDimmed]         = ImVec4(0.18f, 0.08f, 0.10f, 1.00f);
    c[ImGuiCol_TabDimmedSelected] = ImVec4(0.42f, 0.15f, 0.20f, 1.00f);

    // ------------------------------------------------------------------------
    // Stem green accents
    // ------------------------------------------------------------------------

    c[ImGuiCol_CheckMark] = ImVec4(0.55f, 0.72f, 0.46f, 1.00f);

    c[ImGuiCol_SliderGrab]       = ImVec4(0.43f, 0.58f, 0.38f, 1.00f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.58f, 0.75f, 0.48f, 1.00f);

    c[ImGuiCol_ResizeGrip]        = ImVec4(0.45f, 0.58f, 0.39f, 0.50f);
    c[ImGuiCol_ResizeGripHovered] = ImVec4(0.55f, 0.72f, 0.46f, 0.80f);
    c[ImGuiCol_ResizeGripActive]  = ImVec4(0.65f, 0.82f, 0.56f, 1.00f);

    // ------------------------------------------------------------------------
    // Scrollbars
    // ------------------------------------------------------------------------

    c[ImGuiCol_ScrollbarBg]          = ImVec4(0.09f, 0.05f, 0.06f, 1.00f);
    c[ImGuiCol_ScrollbarGrab]        = ImVec4(0.42f, 0.19f, 0.24f, 1.00f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.62f, 0.25f, 0.33f, 1.00f);
    c[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.78f, 0.30f, 0.41f, 1.00f);

    // ------------------------------------------------------------------------
    // Selection
    // ------------------------------------------------------------------------

    c[ImGuiCol_TextSelectedBg] = ImVec4(0.76f, 0.23f, 0.33f, 0.35f);
}

#endif
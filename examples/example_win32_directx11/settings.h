#pragma once
#include "colors_widgets.h"

namespace settings {

    inline ImVec2 size_menu = ImVec2(640, 520);
    inline ImVec2 size_watermark = ImVec2(479, 50);
    inline ImVec2 size_preview = ImVec2(300, 400);

}

namespace misc {

    int tab_count, active_tab_count = 0;

    float anim_tab = 0;

    int tab_width = 85;

    float child_add, alpha_child = 0;

}

namespace menu {

    inline ImVec4 general_child = ImColor(23, 23, 25);

}

namespace pictures {

    inline ID3D11ShaderResourceView* logo_img = nullptr;
    inline ID3D11ShaderResourceView* aim_img = nullptr;
    inline ID3D11ShaderResourceView* misc_img = nullptr;
    inline ID3D11ShaderResourceView* visual_img = nullptr;
    inline ID3D11ShaderResourceView* silentaim_img = nullptr;
    inline ID3D11ShaderResourceView* trigger_img = nullptr;
    inline ID3D11ShaderResourceView* world_img = nullptr;
    inline ID3D11ShaderResourceView* settings_img = nullptr;
    inline ID3D11ShaderResourceView* pen_img = nullptr;
    inline ID3D11ShaderResourceView* keyboard_img = nullptr;
    inline ID3D11ShaderResourceView* input_img = nullptr;
    inline ID3D11ShaderResourceView* wat_logo_img = nullptr;
    inline ID3D11ShaderResourceView* fps_img = nullptr;
    inline ID3D11ShaderResourceView* player_img = nullptr;
    inline ID3D11ShaderResourceView* time_img = nullptr;

}

namespace fonts {

    ImFont* inter_font;

    ImFont* inter_bold_font;

    ImFont* inter_bold_font2;

    ImFont* inter_bold_font3;

    ImFont* inter_bold_font4;

    ImFont* inter_font_b;

    ImFont* combo_icon_font;

    ImFont* weapon_font;

}

namespace features {

    bool check1, check2, check3, check4, check5, check6, check7;

    int sliderint, sliderint2, sliderint3, sliderint4;

    int selectedItem, selected = 0;

    const char* items[]{ "Value", "Random" };

    const char* items_count[]{ "Combo1", "Combo2", "Combo3", "Combo4" };

    inline ImVec4 fov_color = ImColor(60, 157, 173);

    int key, mind = 1;

    int key2, mind2 = 0;

    int key3, mind3 = 0;

    char input[64] = { "" };

    static bool multi[5] = { false, true, true, false, true };

    const char* multi_items[5] = { "Head", "Chest", "Stromatch", "Body", "Legs" };

    static bool multi_esp[7] = { false, false, false, false, false, false, false };

    const char* multi_preview[7] = { "Box", "Health", "Armor", "Nickname", "Distance", "Weapon", "Skeleton" };

    bool esp_perview;

    bool watermark = true;

    float preview_alpha;
}


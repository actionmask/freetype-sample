#include <FontAtlas.h>

#include <FTFontChar.h>
#include <FTFont.h>
#include <TreeNode.h>
#include <OpenGL.h>
#include <Log.h>

#include <ft2build.h>
#include <ftglyph.h>
#include FT_FREETYPE_H

#include <algorithm>
#include <freetype.h>

using std::sort;

#define LOG_TAG "FontAtlas"

FontAtlas::FontAtlas(int width, int height)
{
    this->width = width;
    this->height = height;

    if (FT_Init_FreeType(&library))
    {
        LOGE("Error inializing Freetype2");
    }

    glGenTextures(1, &textureId);
}

FontAtlas::~FontAtlas()
{
    fontList.clear();
    fontCharList.clear();

    if (library)
    {
        FT_Done_FreeType(library);
        library = nullptr;
    }
}

shared_ptr<FTFont> FontAtlas::addFont(const string& fontName, unsigned int size, const string& letters)
{
    FT_Face face;

    string file = "assets/" + fontName;
    if (FT_Error error = FT_New_Face(library, file.c_str(), 0, &face))
    {
        LOGE("Failed to load font.");
        return shared_ptr<FTFont>();
    }

    // FreeType Measures Font Size In Terms Of 1/64ths Of Pixels.
    FT_Set_Char_Size(face, size * 64, size * 64, 72, 72);

    unsigned long len = letters.size();

    shared_ptr<FTFont> font = shared_ptr<FTFont>(new FTFont(face, textureId));
    fontList.push_back(font);

    FTFontChar* fontChar;
    FT_Glyph pGlyph;
    int ixGlyph;

    for (int n = 0; n < len; n++)
    {
        char c = letters[n];

        if (font->getChar(c) == NULL)
        {
            ixGlyph = FT_Get_Char_Index(face, c);
            if (ixGlyph == 0)
            {
                LOGE("character doesn't exist in font: %c", c);
            }
            else
            {
                if (FT_Load_Glyph(face, ixGlyph, FT_LOAD_RENDER))
                {
                    LOGE("Failed to load the glyph for char c=%c.", c);
                }

                // Move The Face's Glyph Into A Glyph Object.
                if (FT_Get_Glyph(face->glyph, &pGlyph))
                {
                    LOGE("Failed to load the glyph object for char c=%c.", c);
                }


                // all metrics dimensions are multiplied by 64, so we have to divide by 64
                int xOffset = (int) face->glyph->metrics.horiBearingX >> 6;
                int yOffset = (int) face->glyph->metrics.horiBearingY >> 6;
                int width = (int) face->glyph->metrics.width >> 6;
                int height = (int) face->glyph->metrics.height >> 6;
                int xAdvance = (int) face->glyph->metrics.horiAdvance >> 6;

                fontChar = new FTFontChar(c, width, height, xOffset, yOffset, xAdvance, pGlyph);
                fontCharList.push_back(fontChar);
                font->addChar(c, fontChar);
            }
        }
    }

    return font;
}

bool greaterSizeComparator(FTFontChar* fontChar1, FTFontChar* fontChar2)
{
    return fontChar1->getNumPixels() > fontChar2->getNumPixels();
}

void FontAtlas::create()
{
    int totalPixels = 0;
    for (int n = 0; n < (int) fontList.size(); n++)
    {
        shared_ptr<FTFont> font = fontList[n];
        totalPixels += font->getTotalNumPixels();
    }

    sort(fontCharList.begin(), fontCharList.end(), greaterSizeComparator);

    TreeNode::getPool().init((int) (fontCharList.size() + 1) * 2);
    if (!binPack(width, height))
    {
        LOGE("Failed to render glyphs to texture");
    }
    TreeNode::getPool().release();

    unsigned char* data = new unsigned char[width * height];
    for (int n = 0; n < (int) fontCharList.size(); n++)
    {
        fontCharList[n]->drawToBitmap(data, width, height);
        fontCharList[n]->releaseGlyph();
    }

    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    int err = glGetError();
    if (err != GL_NO_ERROR)
    {
        LOGE("Error in glTexImage2D: %i", err);
    }

    // clean up memory
    delete[] data;
}

bool FontAtlas::binPack(int texWidth, int texHeight)
{
    TreeNode* treeNode = TreeNode::getPool().allocate();
    treeNode->set(0, 0, texWidth, texHeight);
    for (int n = 0; n < (int) fontCharList.size(); n++)
    {
        if (!treeNode->add(fontCharList[n]))
        {
            return false;
        }
    }
    return true;
}
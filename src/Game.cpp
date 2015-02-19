#include <Game.h>
#include <ShaderProgram.h>
#include <FontAtlas.h>
#include <BufferObject.h>
#include <FontBatchRenderer.h>

#include <iostream>

#define LOG_TAG "Game"

Game::Game()
{
}

void Game::init()
{
    glClearColor(1.0f, 0.20f, 1.0f, 0.60f);

    FontBatchRenderer::getRenderer().init();

    fontAtlas = shared_ptr<FontAtlas>(new FontAtlas(true));
    font = fontAtlas->addFont("LiberationMono-Regular.ttf", 12, " !\"#&'()*,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ\\_abcdefghijklmnopqrstuvwxyz");
    fontAtlas->create();
}

void Game::render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    font->drawString(10, 10, "NEON GENESIS EVANGELION", 0x000000, 1.0f);

    FontBatchRenderer::getRenderer().render();
}

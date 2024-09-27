#include <stdio.h>
#include <box2d.h>
#include "st7789.h"
#include "driver/gpio.h"

TFT_t dev;
b2World *myWorld = NULL;

class TFTDebugDraw : public b2Draw {
    void DrawPolygon(const b2Vec2 *vertices, int32 vertexCount, const b2Color &color) {}

    void DrawSolidPolygon(const b2Vec2 *vertices, int32 vertexCount, const b2Color &color)
    {
        for (int i = 0; i < vertexCount; i++) {
            auto p1 = vertices[i];
            auto p2 = vertices[(i + 1) % vertexCount];
            //lcdDrawLine(&dev, (uint16_t)(p1.x * 10), (uint16_t)(p1.y * 10), (uint16_t)(p2.x * 10), (uint16_t)(p2.y * 10), YELLOW);
        }
    }

    void DrawCircle(const b2Vec2 &center, float radius, const b2Color &color) {}

    void DrawSolidCircle(const b2Vec2 &center, float radius, const b2Vec2 &axis, const b2Color &color)
    {
        //lcdDrawCircle(&dev, (uint16_t)(center.x * 10), (uint16_t)(center.y * 10), (int)(radius * 10), BLUE);
        b2Vec2 p = center + radius * axis;
        //lcdDrawLine(&dev, (uint16_t)(center.x * 10), (uint16_t)(center.y * 10), (uint16_t)(p.x * 10), (uint16_t)(p.y * 10), WHITE);
    }

    void DrawSegment(const b2Vec2 &p1, const b2Vec2 &p2, const b2Color &color) {}

    void DrawTransform(const b2Transform &xf) {}

    void DrawPoint(const b2Vec2 &p, float size, const b2Color &color) {}
};


void createSomeBox()
{
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.allowSleep = true;
    auto w = (random() % 100) / 100.01;
    bodyDef.position.Set(17 + w, 3 + w);
    auto *body = myWorld->CreateBody(&bodyDef);

    b2PolygonShape shape;
    shape.SetAsBox(0.6 + w, 0.6 + w);

    b2FixtureDef f;
    f.shape = &shape;
    f.density = 0.8;
    f.friction = 0.4;
    f.restitution = 0.6;

    body->CreateFixture(&f);
}

extern "C" void app_main(void)
{
    spi_master_init(&dev, GPIO_NUM_47, GPIO_NUM_21, GPIO_NUM_14, GPIO_NUM_45, -1, GPIO_NUM_48);
    lcdInit(&dev, 240, 240, 0, 0);
    createSomeBox();
}

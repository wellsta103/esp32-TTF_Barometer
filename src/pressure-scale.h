

//====================================================
// setup_pressure_scales: Draws the six pressure
// analog meters on screen.
//====================================================
void setup_pressure_scales(const char *label, int x, int y)
{
  int w = 36;
  tft.drawRect(x, y, w, 155, TFT_GREY);
  tft.fillRect(x + 2, y + 19, w - 3, 155 - 38, TFT_WHITE);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString(label, x + w / 2, y + 2, 2);

  for (int i = 0; i < 110; i += 10)
  {
    tft.drawFastHLine(x + 20, y + 27 + i, 6, TFT_BLACK);
  }

  for (int i = 0; i < 110; i += 50)
  {
    tft.drawFastHLine(x + 20, y + 27 + i, 9, TFT_BLACK);
  }

  tft.fillTriangle(x + 3, y + 127, x + 3 + 16, y + 127, x + 3, y + 127 - 5, TFT_RED);
  tft.fillTriangle(x + 3, y + 127, x + 3 + 16, y + 127, x + 3, y + 127 + 5, TFT_RED);

  tft.drawCentreString("---", x + w / 2, y + 155 - 18, 2);
}

//====================================================
// update_pressure_arrows: Draws the six small pressure
// arrows on screen, and below the relative pressure vs.
// 1013 as number for every scale.
//====================================================
void update_pressure_arrows(void)
{
  int dy = 187;
  byte pw = 16;

  // This will be the 0 pressure reference, aka 'now'-value for all other array pressures to relate the difference
  // float now_value = value[5]/10 ;

  tft.setTextColor(TFT_GREEN, TFT_BLACK);

#if MYDEBUG == 1
  debugln();
  for (int i = 0; i < 6; i++)
  {
    Serial.printf("value[%d]: %d ", i, value[i]);
  }
  debugln();
#endif

  for (int i = 0; i < 6; i++)
  {

    // Put text values, divide by 10, and refer to 'now' gives each a 'now'-difference in the scale marker in the small boxes below each scale

    char buf[8];
    dtostrf(value[i], 4, 0, buf);
    tft.drawRightString(buf, i * 40 + 36 - 5, 187 - 27 + 155 - 18, 2);
    Serial.printf("TAW5: %s %d\n", buf, value[i]);

    // Move the 6 pointers one pixel towards new value
    int dx = 3 + 40 * i;
    if (value[i] < 0)
      value[i] = 0; // Limit value to emulate needle end stops
    if (value[i] > 100)
      value[i] = 100;

    while (!(value[i] == old_value[i]))
    {
      dy = 187 + 100 - old_value[i];
      if (old_value[i] > value[i])
      {
        tft.drawLine(dx, dy - 5, dx + pw, dy, TFT_WHITE);
        old_value[i]--;
        tft.drawLine(dx, dy + 6, dx + pw, dy + 1, TFT_RED);
      }
      else
      {
        tft.drawLine(dx, dy + 5, dx + pw, dy, TFT_WHITE);
        old_value[i]++;
        tft.drawLine(dx, dy - 6, dx + pw, dy - 1, TFT_RED);
      }
    }
  }
}
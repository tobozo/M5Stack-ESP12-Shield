

#define lineCharBufferWidth 132 // text columns
char *scrollLine = new char[lineCharBufferWidth];
char *scrollLineBuf = new char[lineCharBufferWidth+1];
uint8_t scrollLineBufferPos = 0;
#define TFT_LIGHTGREY tft.color565(192,192,192)
static int lastBaudRate = 0;
static uint32_t lastRxBytes = 0;
static uint32_t lastTxBytes = 0;

const int textSize = 1; // scrolled text size
const int TFA = 14;
const int BFA = 56;
int scrollPos = TFA;
int areaHeight = 0;
int blockHeight = 0;
uint8_t hmargin = 8;


void setupScrollArea(int32_t tfa, int32_t vsa, int32_t bfa)
{
  tft.writecommand(0x33);  // ILI9341_VSCRDEF Vertical scroll definition
  tft.writedata(tfa >> 8); // Top Fixed Area line count
  tft.writedata(tfa);
  tft.writedata(vsa >> 8); // Vertical Scrolling Area line count
  tft.writedata(vsa);
  tft.writedata(bfa >> 8); // Bottom Fixed Area line count
  tft.writedata(bfa);
  log_d("Init Hardware Scroll area with tfa/vsa/bfa %d/%d/%d on w/h %d/%d", tfa, vsa, bfa, tft.width(), tft.height());
}


void scrollTo(uint16_t vsp)
{
  tft.writecommand(0x37);  // ILI9341_VSCRSADD Vertical scrolling pointer
  tft.writedata(vsp>>8);
  tft.writedata(vsp);
}


void modulo_scroll( int &pos )
{
  if( pos >= tft.height()-(BFA) ) {
    int diff = pos - (tft.height()-(BFA));
    pos = TFA + diff;
  }
  if( pos < TFA ) {
    pos += areaHeight;
  }
}


void scroll_lines( uint32_t amount, uint32_t waitdelay=10 )
{
  if( amount == 0 ) return;
  for( int i=0; i<amount; i++ ) {
    scrollPos++;
    modulo_scroll( scrollPos );
    tft.drawFastHLine( hmargin, scrollPos, tft.width()-hmargin*2, TFT_BLACK ); // clear top line before it warps to the bottom
    scrollTo( scrollPos );
    delay( waitdelay );
  }
}



void displayScrollTitle( int baudRate, uint32_t rxBytes, uint32_t txBytes, int32_t bgcolor=TFT_WHITE, bool force = false )
{

  if( bgcolor < 0 ) {
    tft.setTextColor( TFT_BLACK ); // transparent
  } else {
    tft.setTextColor( TFT_BLACK, bgcolor);
  }
  tft.setTextDatum( ML_DATUM );
  tft.setFont( nullptr );
  tft.setTextSize( 1 );

  if( force || lastBaudRate != baudRate ) {
    lastBaudRate = baudRate;
    tft.setCursor( 18, 6 );
    tft.printf("%d   ", baudRate);
  }
  if( force || rxBytes != lastRxBytes ) {
    lastRxBytes = rxBytes;
    tft.setCursor( 100, 6 );
    tft.printf("RX:%d ", rxBytes );
  }
  if( force || txBytes != lastTxBytes ) {
    lastTxBytes = txBytes;
    tft.setCursor( 200, 6 );
    tft.printf("TX:%d ", txBytes );
  }
}


void scrollReset()
{
  scrollTo( 0 );
  tft.fillRect( 0, 0, tft.width(), areaHeight+blockHeight+TFA, TFT_BLACK ); // clear zone
  tft.drawJpg( doc_intrologo_jpg, doc_intrologo_jpg_len, 0,  TFA-1); // intro bitmap
  tft.fillRect( 0, 0, tft.width(), TFA-1, TFT_WHITE ); // titlebar background fill
  displayScrollTitle( lastBaudRate, lastRxBytes, lastTxBytes, TFT_WHITE, true ); // put some text in the titlebar
  tft.fillRect( 0, TFA-1, hmargin, areaHeight+blockHeight, TFT_BLACK ); // scrollzone marginleft
  tft.fillRect( tft.width()-(hmargin+1), TFA-1, hmargin, areaHeight+blockHeight, TFT_BLACK ); // scrollzone marginright
  tft.drawRect( 0, TFA-1, tft.width(), areaHeight+blockHeight-4,   TFT_DARKGREY ); // scrollzone border
  tft.drawFastHLine( hmargin, scrollPos, tft.width()-hmargin*2, TFT_BLACK ); // clear top line before it warps to the bottom
}




void scrollSetup()
{
  // compute area height
  areaHeight = tft.height()-(TFA+BFA);
  if( areaHeight%2 != 0 ) {
    Serial.printf("Scroll area height is odd (%d lines), aborting\n", areaHeight);
    while(1);
  }
  setupScrollArea( 0, 0, 0 ); // reset
  setupScrollArea( TFA, areaHeight, BFA );

  tft.setFont(nullptr);
  tft.setTextSize( textSize );

  blockHeight = tft.fontHeight() + 2; // !! areaHeight must be a multiple of blockHeight

  scrollReset();

  if( areaHeight/blockHeight != float( (areaHeight*100.0) / (blockHeight*100.0) ) ) {
    Serial.printf("areaHeight (%d) isn't a multiple of blockHeight (%d), aborting\n", areaHeight, blockHeight );
    while(1);
  }
}






void scrollText( const char* text, uint32_t waitdelay=10, uint16_t color=TFT_GREEN  )
{
  if( text[0] == '\0' ) return;
  scroll_lines( blockHeight, waitdelay );
  int textVPos = scrollPos + ( areaHeight - blockHeight ); // bottom aligned
  modulo_scroll( textVPos );
  tft.setFont(nullptr);
  tft.setTextSize( textSize );
  tft.setTextDatum( TL_DATUM);
  tft.setTextColor(color, TFT_BLACK);
  tft.drawString( text, hmargin, textVPos );
}


uint8_t chunkScroll( uint32_t waitdelay=10, uint16_t color=TFT_GREEN )
{
  tft.setFont(nullptr);
  tft.setTextSize( textSize );
  for( uint8_t i=0; i<=scrollLineBufferPos; i++ ) {
    scrollLineBuf[i] = scrollLine[i];
    scrollLineBuf[i+1] = '\0'; // null terminate
    int w = tft.textWidth( scrollLineBuf );
    if( w > tft.width()-hmargin*2 ) { // text exceeds width, flush to screen
      char to_report = scrollLineBuf[i];
      scrollLineBuf[i] = '\0'; // remove last letter
      scrollText( scrollLineBuf, waitdelay, color ); // print and scroll

      uint8_t j, k;
      scrollLine[0] = to_report;
      for( j=i,k=1;j<=scrollLineBufferPos;j++,k++ ) {
        scrollLine[k] = scrollLine[j];
      }
      scrollLine[k+1] = '\0'; // null terminate
      return k;//scrollLineBufferPos - i;
    }
  }
  if( scrollLineBuf[scrollLineBufferPos] == '\n' ) {
    scrollLineBuf[scrollLineBufferPos] = '\0';
    scrollText( scrollLineBuf, waitdelay, color ); // print and scroll
    return 0;
  }
  return scrollLineBufferPos+1; // increment
}


void scrollLinePushChar( char c, uint16_t color=TFT_GREEN )
{
  int waitdelay = 0;
  scrollLine[scrollLineBufferPos] = c;
  uint8_t newPos = chunkScroll( waitdelay, color );

  if( newPos > lineCharBufferWidth ) {
    scrollText( scrollLine, waitdelay, color );
    scrollLine[0] = '\0';
    scrollLineBufferPos = 0;
  } else {
    scrollLineBufferPos = newPos;
  }
}

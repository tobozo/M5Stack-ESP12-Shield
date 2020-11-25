

#define lineCharBufferWidth 132 // text columns
char *scrollLine = new char[lineCharBufferWidth];
char *scrollLineBuf = new char[lineCharBufferWidth+1];
uint8_t scrollLineBufferPos = 0;

const int textSize = 1; // scrolled text size
const int TFA = 4;
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


void scrollReset()
{
  scrollTo( 0 );
  tft.fillRect( 0, 0, tft.width(), areaHeight+blockHeight+TFA, TFT_BLACK );
  tft.drawRect( 0, 0, tft.width(), areaHeight+blockHeight-2,   TFT_DARKGREY );
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







void scrollText( const char* text, uint32_t waitdelay=10 )
{
  if( text[0] == '\0' ) return;
  scroll_lines( blockHeight, waitdelay );
  int textVPos = scrollPos + ( areaHeight - blockHeight ); // bottom aligned
  modulo_scroll( textVPos );
  tft.setFont(nullptr);
  tft.setTextSize( textSize );
  tft.setTextDatum( TL_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString( text, hmargin, textVPos );
}


uint8_t chunkScroll( uint32_t waitdelay=10 )
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
      scrollText( scrollLineBuf, waitdelay ); // print and scroll

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
    scrollText( scrollLineBuf, waitdelay ); // print and scroll
    return 0;
  }
  return scrollLineBufferPos+1; // increment
}


void scrollLinePushChar( char c )
{
  int waitdelay = 0;
  scrollLine[scrollLineBufferPos] = c;
  uint8_t newPos = chunkScroll( waitdelay );

  if( newPos > lineCharBufferWidth /*|| c == '\n'*/) {
    scrollText( scrollLine, waitdelay );
    scrollLine[0] = '\0';
    scrollLineBufferPos = 0;
  } else {
    scrollLineBufferPos = newPos;
  }
}

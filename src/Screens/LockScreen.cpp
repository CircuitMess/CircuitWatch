
#include <Util/Debug.h>
#include <Update/UpdateManager.h>
#include "../CircuitWatch.h"
#include "LockScreen.h"
#include <Bitmaps/Bitmaps.hpp>
#include "../Bitmaps/Bitmaps.hpp"

LockScreen* LockScreen::instance = nullptr;

LockScreen::LockScreen(Display& display, Context* unlockedScreen) :
		Context(display), unlockedScreen(unlockedScreen),
		layers(&screen),
		bgScroll(&layers),
		bgLayoutCache(&bgScroll),
		bgLayout(&bgLayoutCache, HORIZONTAL),
		bgGrid(&bgLayout, 2),
		bgImage0(&bgLayout, 28, 64),
		bgImage1(&bgGrid, 24, 31),
		bgImage2(&bgGrid, 36, 31),
		fgLayout(&layers, VERTICAL),
		clock(&fgLayout, screen.getWidth() - 10, screen.getHeight() - 28),
		lock(&fgLayout, 18, 18){

	addSprite(&bgLayoutCache);
	addSprite(&bgImage0);
	addSprite(&bgImage1);
	addSprite(&bgImage2);
	addSprite(&clock);
	addSprite(&lock);

	instance = this;

	/*rtc_gpio_isolate(GPIO_NUM_16);
	rtc_gpio_isolate(GPIO_NUM_17);
	rtc_gpio_isolate(GPIO_NUM_33);
	rtc_gpio_isolate(GPIO_NUM_34);
	rtc_gpio_isolate(GPIO_NUM_36);
	rtc_gpio_isolate(GPIO_NUM_39);

	RTC.begin();*/

	/** clockYear, clockMonth, clockDay, clockHour, clockMinute, clockSecond */
	/*DateTime now = DateTime(2020, 3, 22, 15, 0, 0);
	RTC.adjust(now);*/


	buildUI();
	pack();
}

void LockScreen::btnXPress(){
	if(instance == nullptr) return;

	instance->lockTimer = 0;
	instance->sleepTimer = 0;

	if(instance->sleep){
		instance->sleep = false;
		UpdateManager::addListener(instance);
		instance->getScreen().getDisplay()->setPower(true);
	}

	instance->btnState = true;
	instance->lock.getSprite()->clear(TFT_TRANSPARENT);
	instance->lock.getSprite()->drawIcon(lock_open, 0, 0, 18, 18, 1);
}

void LockScreen::btnXRelease(){
	if(instance == nullptr) return;

	uint score = instance->btnStateTime;

	instance->btnState = false;
	instance->btnStateTime = 0;

	instance->lock.getSprite()->clear(TFT_TRANSPARENT);
	instance->lock.getSprite()->drawIcon(lock_closed, 0, 0, 18, 18, 1);

	// under 10 pixel move
	if((score > 40 * instance->unlockSpeed) && (score < 100 * instance->unlockSpeed)){
		// sleep


		if(!instance->sleep){
			instance->sleep = true;
			UpdateManager::removeListener(instance);
			/*instance->layers.getSprite()->clear(TFT_BLACK);
			instance->layers.draw();
			instance->screenDrawn = false;*/
			instance->screen.getDisplay()->setPower(false);
		}
	}
}

void LockScreen::start(){
	Input::getInstance()->setBtnPressCallback(BTN_D, btnXPress);
	Input::getInstance()->setBtnReleaseCallback(BTN_D, btnXRelease);

	UpdateManager::addListener(this);

	lockTimer = 0;
	sleepTimer = 0;
}

void LockScreen::stop(){
	Input::getInstance()->removeBtnPressCallback(BTN_D);
	Input::getInstance()->removeBtnReleaseCallback(BTN_D);

	UpdateManager::removeListener(this);
}

void LockScreen::pack(){
	Context::pack();
}

void LockScreen::unpack(){
	Context::unpack();

	bgImage0.getSprite()->clear(TFT_GREEN);
	bgImage1.getSprite()->clear(TFT_GREEN);
	bgImage2.getSprite()->clear(TFT_GREEN);
	clock.getSprite()->clear(TFT_TRANSPARENT);
	lock.getSprite()->clear(TFT_TRANSPARENT);
	lock.getSprite()->drawIcon(lock_closed, 0, 0, 18, 18, 1);
	bgLayoutCache.refresh();
}

void LockScreen::update(uint time){
	if(packed || sleep) return;

	lockTimer += time;
	sleepTimer += time;

	// sleep timer
	if(false && sleepTimer > 5000 && !instance->sleep){
		instance->sleep = true;
		UpdateManager::removeListener(instance);
		instance->screen.getDisplay()->setPower(false);
	}

	if(btnState){
		btnStateTime += time;
	}

	if(btnStateTime / unlockSpeed > (fgLayout.getAvailableWidth() - lock.getWidth()) && unlockedScreen != nullptr){
		btnState = false;
		btnStateTime = 0;

		unlockedScreen->push(this);

		return;
	}

	draw();
	screen.commit();

	//delay(20);
}

#define RGB(R, G, B) (((R & 0xF8) << 8) | ((G & 0xFC) << 3) | (B >> 3))

void LockScreen::draw(){
	screen.clear();

	uint m = millis();

	/** BG */

	uint factor = 40000; // 5000;
	float speed = M_PI; // 13.0f / 7.0f;
	uint i = (float) abs(((int) (m * speed) % (factor * 2)) - factor) / factor * (float) bgScroll.getMaxScrollX();

	bgScroll.setScroll(i, 0);
	bgScroll.setPos(0, 0);
	bgScroll.draw();

	bgScroll.setScroll(bgScroll.getMaxScrollX() - i, 0);
	bgScroll.setPos(0, 66);
	bgScroll.draw();

#define nf 0.5
#define nfb fabs(2.0 * ((int) (bp * nf) % 255) - 255)
#define clr RGB(190, (240-bp/4), (20+bp/3))

	/** Checkers */
	Sprite* checkerSprite = screen.getSprite();

	uint mdiffCol = 2000;
	uint mdiffPos = 500;
	uint rects = 15;
	float rectSpace = 0.5;
	float boxSize = 128.0 / rects;
	uint rectSize = rectSpace * boxSize;

	float mt = fabs(2.0 * (m % mdiffCol) / mdiffCol - 1.0);
	//float mt = cos(2.0 * M_PI * m / mdiffCol) / 2.0 + 1.0;

	for(int i = 0; i < rects+2; i++){
		for(int j = 0; j < rects+2; j++){
			float pt = (i + j) / 2.0 / rects;
			//pt = sin(2 * M_PI * pt) / 2.0 + 0.5;

			uint bp = (byte) ((mt + cos(pt) / 2.0 + 0.5) / 2.0 * 254.0);
			//uint bp = (byte) ((mt + pt) / 2.0 * 254.0);

			float t = (float) (m % mdiffPos) / mdiffPos;
			int posDiff = t * boxSize - boxSize;
			int posX = i * boxSize + posDiff;
			int posY = j * boxSize + posDiff;
			checkerSprite->fillRect(posX, posY, rectSize, rectSize, clr);
		}
	}

	/** Clock */
	clock.getSprite()->clear(TFT_TRANSPARENT);
	clock.getSprite()->setTextFont(1);

	DateTime currentTime(m / 1000); // RTC.now();
	uint sec = currentTime.second() % 60;
	uint min = currentTime.minute();
	uint hour = currentTime.hour();
	uint day = currentTime.day();
	uint month = currentTime.month();
	uint year = currentTime.year();

	// hour / minute
	clock.getSprite()->setTextSize(5);
	clock.getSprite()->setTextColor(TFT_PURPLE);
	clock.getSprite()->setCursor(2, 23);
	if(hour < 10) clock.getSprite()->print("0");
	clock.getSprite()->println(String(hour));
	clock.getSprite()->setCursor(2, clock.getSprite()->getCursorY()-1);
	if(min < 10) clock.getSprite()->print("0");
	clock.getSprite()->println(String(min));

	// track
	clock.getSprite()->fillRoundRect(62 + 15 - (float) abs(((m * 2 + 1) % 2000) - 1000) * 15.0f / 1000.0f, 62, 40, 6, 2, TFT_PURPLE);

	// date
	clock.getSprite()->setCursor(75 + (day < 10) * 2, 72);
	clock.getSprite()->setTextColor(TFT_BLUE);
	clock.getSprite()->setTextSize(1);
	clock.getSprite()->print(String(day) + "/");
	if(month < 10) clock.getSprite()->print("0");
	clock.getSprite()->println(String(month) + "");

	// year
	clock.getSprite()->setCursor(77, clock.getSprite()->getCursorY() + 2);
	clock.getSprite()->println(String(year));

	// seconds
	clock.getSprite()->setCursor(78, 43);
	clock.getSprite()->setTextColor(TFT_RED);
	clock.getSprite()->setTextSize(2);
	if(sec < 10) clock.getSprite()->print("0");
	clock.getSprite()->println(String(sec));

	if(lockTimer < 2000){
		uint newPos = lock.getSprite()->getX();
		if(fgLayout.getAvailableWidth() >= lock.getWidth() + btnStateTime / unlockSpeed){
			newPos = fgLayout.getAvailableWidth() - lock.getWidth() + fgLayout.getPadding() - btnStateTime / unlockSpeed;
		}
		lock.setPos(newPos, lock.getY());
		Serial.println(String(newPos));
	}

	fgLayout.draw();
}

void LockScreen::buildUI(){
	layers.setWHType(PARENT, PARENT);
	layers.reflow();

	/** BG */
	bgImage0.getSprite()->clear(TFT_GREEN);
	bgImage1.getSprite()->clear(TFT_GREEN);
	bgImage2.getSprite()->clear(TFT_GREEN);

	bgScroll.setWHType(PARENT, CHILDREN);

	bgLayout.setWHType(CHILDREN, CHILDREN);
	bgLayout.setPadding(0);
	bgLayout.setGutter(2);


	bgGrid.setWHType(CHILDREN, CHILDREN);
	bgGrid.setGutter(2);

	bgGrid.addChild(&bgImage1);
	bgGrid.addChild(&bgImage2);
	bgGrid.addChild(&bgImage2);
	bgGrid.addChild(&bgImage1);
	bgGrid.reflow();
	bgGrid.setStrictPos(true);

	bgLayout.addChild(&bgImage0);
	bgLayout.addChild(&bgGrid);
	bgLayout.addChild(&bgImage0);
	bgLayout.addChild(&bgGrid);
	bgLayout.addChild(&bgImage0);
	bgLayout.addChild(&bgGrid);
	bgLayout.addChild(&bgImage0);
	bgLayout.addChild(&bgGrid);
	bgLayout.addChild(&bgImage0);
	bgLayout.reflow();
	bgLayout.setStrictPos(true);

	bgLayoutCache.setWHType(CHILDREN, CHILDREN);
	bgLayoutCache.addChild(&bgLayout);
	bgLayoutCache.reflow();
	bgLayoutCache.repos();
	bgLayoutCache.refresh();

	bgScroll.addChild(&bgLayoutCache);
	bgScroll.reflow();
	// bgScroll.getSprite()->setParent(layers.sprite); // TODO: ??
	layers.addChild(&bgScroll);

	/** FG */
	fgLayout.setWHType(PARENT, PARENT);
	fgLayout.setPadding(5);
	fgLayout.addChild(&clock);
	fgLayout.addChild(&lock);
	fgLayout.reflow();

	clock.getSprite()->clear(TFT_TRANSPARENT).setChroma(TFT_TRANSPARENT);
	lock.getSprite()->clear(TFT_TRANSPARENT).setChroma(TFT_TRANSPARENT);
	lock.getSprite()->drawIcon(lock_closed, 0, 0, 18, 18, 1);

	layers.addChild(&fgLayout);

	screen.addChild(&layers);
	screen.repos();
}

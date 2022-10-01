#include "Free_Fonts.h" // Include the header file attached to this sketch

void setup_screen() {
	// Initialise the TFT registers
	tft.init();
	tft.setRotation(1);

	// Optionally set colour depth to 8 or 16 bits, default is 16 if not specified
	spr.setColorDepth(8);

	// Create a sprite of defined size
	spr.createSprite(WIDTH, HEIGHT);
	// Clear the TFT screen to black
	tft.fillScreen(TFT_BLACK);
}

int data[280] = { 0 };

float to_scale(float reading) {
	float temp = HEIGHT -
		(
			(
				(
					(float)((reading - 20480.0) / 4095.0)
					+ (offset / 3.3)
					)
				* 3300 /
				(v_div * 6)
				)
			)
		* (HEIGHT - 1)
		- 1;
	return temp;
}

float to_voltage(float reading) {
	return  (reading - 20480.0) / 4095.0 * 3.3;
}

uint32_t from_voltage(float voltage) {
	return uint32_t(voltage / 3.3 * 4095 + 20480.0);
}

void update_screen(uint16_t* i2s_buff, float sample_rate) {

	float mean = 0;
	float max_v, min_v;

	peak_mean(i2s_buff, BUFF_SIZE, &max_v, &min_v, &mean);

	float freq = 0;
	float period = 0;
	uint32_t trigger0 = 0;
	uint32_t trigger1 = 0;

	//if analog mode OR auto mode and wave recognized as analog
	bool digital_data = false;
	if (digital_wave_option == 1) {
		trigger_freq_analog(i2s_buff, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0, &trigger1);
	}
	else if (digital_wave_option == 0) {
		digital_data = digital_analog(i2s_buff, max_v, min_v);
		if (!digital_data) {
			trigger_freq_analog(i2s_buff, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0, &trigger1);
		}
		else {
			trigger_freq_digital(i2s_buff, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0);
		}
	}
	else {
		trigger_freq_digital(i2s_buff, sample_rate, mean, max_v, min_v, &freq, &period, &trigger0);
	}

	draw_sprite(freq, period, mean, max_v, min_v, trigger0, sample_rate, digital_data, true);
}

void draw_sprite(float freq,
	float period,
	float mean,
	float max_v,
	float min_v,
	uint32_t trigger,
	float sample_rate,
	bool digital_data,
	bool new_data
) {

	max_v = to_voltage(max_v);
	min_v = to_voltage(min_v);

	String frequency = "";
	if (freq < 1000)
		frequency = String(freq) + "Hz";
	else if (freq < 100000)
		frequency = String(freq / 1000) + "kHz";
	else
		frequency = "----";

	String s_mean = "";
	if (mean > 1.0)
		s_mean = "Avg: " + String(mean) + "V";
	else
		s_mean = "Avg: " + String(mean * 1000.0) + "mV";

	String str_filter = "";
	if (current_filter == 0)
		str_filter = "None";
	else if (current_filter == 1)
		str_filter = "Pixel";
	else if (current_filter == 2)
		str_filter = "Mean-5";
	else if (current_filter == 3)
		str_filter = "Lpass9";

	String str_stop = "";
	if (!stop)
		str_stop = "RUNNING";
	else
		str_stop = "STOPPED";

	String wave_option = "";
	if (digital_wave_option == 0)
		if (digital_data)
			wave_option = "AUTO:Dig/data";
		else
			wave_option = "AUTO:Analog";
	else if (digital_wave_option == 1)
		wave_option = "MODE:Analog";
	else
		wave_option = "MODE:Digital";


	if (new_data) {
		// Fill the whole sprite with black (Sprite is in memory so not visible yet)
		spr.fillSprite(TFT_BLACK);

		draw_grid();

		if (auto_scale) {
			v_div = 1000.0 * max_v / 6.0;
			s_div = period / 3.5;
			if (s_div > 7000 || s_div <= 0)
				s_div = 7000;
			if (v_div <= 0)
				v_div = 825;
		}

		//only draw digital data if a trigger was in the data
		if (!(digital_wave_option == 2 && trigger == 0))
			draw_channel1(trigger, 0, i2s_buff, sample_rate);
	}

	int shift = 150;
	if (menu) {
		spr.setFreeFont(FM9);
		//spr.setTextSize(4);
		uint32_t selectcolor;
		selectcolor = TFT_RED;
		if (set_value) selectcolor = TFT_BLUE;
		spr.drawLine(0, 120, 319, 120, TFT_WHITE); //center line
		spr.fillRect(shift, 0, 160, 215, TFT_BLACK);
		spr.drawRect(shift, 0, 160, 215, TFT_WHITE);
		spr.fillRect(shift + 1, 3 + 15 * (opt - 1), 158, 15, selectcolor);
		spr.setTextColor(TFT_WHITE, (opt == Autoscale) ? selectcolor : TFT_BLACK);
		spr.drawString("AUTOSCALE " + String(auto_scale ? "ON" : "OFF"), shift + 5, 3);
		spr.setTextColor(TFT_WHITE, (opt == Vdiv) ? selectcolor : TFT_BLACK);
		spr.drawString(String(int(v_div)) + "mV/div", shift + 5, 18);
		spr.setTextColor(TFT_WHITE, (opt == Sdiv) ? selectcolor : TFT_BLACK);
		spr.drawString(String(int(s_div)) + "uS/div", shift + 5, 33);
		spr.setTextColor(TFT_WHITE, (opt == Offset) ? selectcolor : TFT_BLACK);
		spr.drawString("Offset: " + String(offset) + "V", shift + 5, 48);
		spr.setTextColor(TFT_WHITE, (opt == TOffset) ? selectcolor : TFT_BLACK);
		spr.drawString("T-Off: " + String((uint32_t)toffset) + "uS", shift + 5, 63);
		spr.setTextColor(TFT_WHITE, (opt == Filter) ? selectcolor : TFT_BLACK);
		spr.drawString("Filter: " + str_filter, shift + 5, 78);
		spr.setTextColor(TFT_WHITE, (opt == Stop) ? selectcolor : TFT_BLACK);
		spr.drawString(str_stop, shift + 5, 93);
		spr.setTextColor(TFT_WHITE, (opt == Mode) ? selectcolor : TFT_BLACK);
		spr.drawString(wave_option, shift + 5, 108);
		spr.setTextColor(TFT_WHITE, (opt == Single) ? selectcolor : TFT_BLACK);
		spr.drawString("Single " + String(single_trigger ? "ON" : "OFF"), shift + 5, 123);
		spr.setTextColor(TFT_WHITE, (opt == Reset) ? selectcolor : TFT_BLACK);
		spr.drawString("Reset ", shift + 5, 138);
		spr.setTextColor(TFT_WHITE, TFT_BLACK);

		spr.drawLine(shift, 158, 310, 158, TFT_WHITE);

		spr.drawString("Vmax: " + String(max_v) + "V", shift + 5, 163);
		spr.drawString("Vmin: " + String(min_v) + "V", shift + 5, 178);
		spr.drawString(s_mean, shift + 5, 193);

		spr.fillRect(25, 0, 125, 40, TFT_BLACK);
		spr.drawRect(25, 0, 125, 40, TFT_WHITE);
		spr.drawString("P-P: " + String(max_v - min_v) + "V", 30, 3);
		spr.drawString(frequency, 30, 20);
		String offset_line = String((2.0 * v_div) / 1000.0 - offset) + "V";
		spr.drawString(offset_line, 30, 60);
	}
	else if (info) {
		spr.setFreeFont(FSS9);
		spr.drawLine(0, 120, 319, 120, TFT_WHITE); //center line
		spr.drawString("P-P: " + String(max_v - min_v) + "V", shift + 15, 5);
		spr.drawString(frequency, shift + 15, 22);
		spr.drawString(String(int(v_div)) + "mV/div", shift - 100, 5);
		spr.drawString(String(int(s_div)) + "uS/div", shift - 100, 22);
		String offset_line = String((2.0 * v_div) / 1000.0 - offset) + "V";
		spr.drawString(offset_line, shift + 100, 112);
	}


	//push the drawed sprite to the screen
	spr.pushSprite(0, 0);

	yield(); // Stop watchdog reset
}

void draw_grid() {
	uint32_t pixelcolor;
	pixelcolor = TFT_DARKGREEN;
	for (int i = 0; i < 32; i++) {
		spr.drawPixel(i * 10, 0, pixelcolor);
		spr.drawPixel(i * 10, 40, pixelcolor);
		spr.drawPixel(i * 10, 80, pixelcolor);
		spr.drawPixel(i * 10, 120, pixelcolor);
		spr.drawPixel(i * 10, 160, pixelcolor);
		spr.drawPixel(i * 10, 200, pixelcolor);
		spr.drawPixel(i * 10, 239, pixelcolor);
	}
	for (int i = 0; i < 240; i += 10) {
		for (int j = 0; j < 320; j += 40) {
			spr.drawPixel(j, i, pixelcolor);
		}
	}
	for (int i = 0; i < 240; i += 10) {
		spr.drawPixel(319, i, pixelcolor);
	}
	spr.drawPixel(319, 239, pixelcolor);
}

void draw_channel1(uint32_t trigger0, uint32_t trigger1, uint16_t* i2s_buff, float sample_rate) {
	//screen wave drawing
	data[0] = to_scale(i2s_buff[trigger0]);
	low_pass filter(0.99);
	mean_filter mfilter(5);
	mfilter.init(i2s_buff[trigger0]);
	filter._value = i2s_buff[trigger0];
	float data_per_pixel = (s_div / 50.0) / (sample_rate / 1000);

	uint32_t linecolor;
	//  uint32_t cursor = (trigger1-trigger0)/data_per_pixel;
	//  spr.drawLine(cursor, 0, cursor, 135, TFT_RED);

	uint32_t index_offset = (uint32_t)(toffset / data_per_pixel);
	trigger0 += index_offset;
	uint32_t old_index = trigger0;
	float n_data = 0, o_data = to_scale(i2s_buff[trigger0]);
	for (uint32_t i = 1; i < 320; i++) {
		uint32_t index = trigger0 + (uint32_t)((i + 1) * data_per_pixel);
		if (index < BUFF_SIZE) {
			if (full_pix && s_div > 50 && current_filter == 0) {
				uint32_t max_val = i2s_buff[old_index];
				uint32_t min_val = i2s_buff[old_index];
				for (int j = old_index; j < index; j++) {
					//draw lines for all this data points on pixel i
					if (i2s_buff[j] > max_val)
						max_val = i2s_buff[j];
					else if (i2s_buff[j] < min_val)
						min_val = i2s_buff[j];

				}
				linecolor = TFT_BLUE;
				spr.drawLine(i, to_scale(min_val), i, to_scale(max_val), linecolor);
			}
			else {
				if (current_filter == 1) {
					linecolor = TFT_RED;
					n_data = to_scale(i2s_buff[index]);
					spr.drawLine(i - 1, o_data, i, n_data, linecolor);
				}
				else if (current_filter == 2) {
					linecolor = TFT_GREEN;
					n_data = to_scale(mfilter.filter((float)i2s_buff[index]));
					spr.drawLine(i - 1, o_data, i, n_data, linecolor);
				}
				else if (current_filter == 3) {
					linecolor = TFT_YELLOW;
					n_data = to_scale(filter.filter((float)i2s_buff[index]));
					spr.drawLine(i - 1, o_data, i, n_data, linecolor);
				}
				else { // undefined
					linecolor = TFT_WHITE;
					n_data = to_scale(i2s_buff[index]);
//					spr.drawLine(i - 1, o_data, i, n_data, linecolor);
					spr.drawPixel(i, n_data, linecolor);
				}
				o_data = n_data;
			}
		}
		else {
			break;
		}
		old_index = index;
	}
}

/*
	Application template for Amazfit Bip BipOS
	(C) Maxim Volkov  2019 <Maxim.N.Volkov@ya.ru>
	
	Шаблон приложения для загрузчика BipOS
	
*/

#include <libbip.h>
#include "main.h"

//	структура меню экрана - для каждого экрана своя
struct regmenu_ screen_data = {
						55,							//	номер главного экрана, значение 0-255, для пользовательских окон лучше брать от 50 и выше
						1,							//	вспомогательный номер экрана (обычно равен 1)
						0,							//	0
						dispatch_screen,			//	указатель на функцию обработки тача, свайпов, долгого нажатия кнопки
						key_press_screen, 			//	указатель на функцию обработки нажатия кнопки
						screen_job,					//	указатель на функцию-колбэк таймера 
						0,							//	0
						show_screen,				//	указатель на функцию отображения экрана
						0,							//	
						0							//	долгое нажатие кнопки
					};
					
int main(int param0, char** argv){	//	здесь переменная argv не определена
	show_screen((void*) param0);
}

void show_screen (void *param0){
struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
struct app_data_ *	app_data;					//	указатель на данные экрана

// проверка источника запуска процедуры
if ( (param0 == *app_data_p) && get_var_menu_overlay()){ // возврат из оверлейного экрана (входящий звонок, уведомление, будильник, цель и т.д.)

	app_data = *app_data_p;					//	указатель на данные необходимо сохранить для исключения 
											//	высвобождения памяти функцией reg_menu
	*app_data_p = NULL;						//	обнуляем указатель для передачи в функцию reg_menu	

	// 	создаем новый экран, при этом указатель temp_buf_2 был равен 0 и память не была высвобождена	
	reg_menu(&screen_data, 0); 				// 	menu_overlay=0
	
	*app_data_p = app_data;						//	восстанавливаем указатель на данные после функции reg_menu
	
	//   здесь проводим действия при возврате из оверлейного экрана: восстанавливаем данные и т.д.

} else { // если запуск функции произошел впервые т.е. из меню 

	// создаем экран (регистрируем в системе)
	reg_menu(&screen_data, 0);

	// выделяем необходимую память и размещаем в ней данные, (память по указателю, хранящемуся по адресу temp_buf_2 высвобождается автоматически функцией reg_menu другого экрана)
	*app_data_p = (struct app_data_ *)pvPortMalloc(sizeof(struct app_data_));
	app_data = *app_data_p;		//	указатель на данные
	
	// очистим память под данные
	_memclr(app_data, sizeof(struct app_data_));
	
	//	значение param0 содержит указатель на данные запущенного процесса структура Elf_proc_
	app_data->proc = param0;
	
	// запомним адрес указателя на функцию в которую необходимо вернуться после завершения данного экрана
	if ( param0 && app_data->proc->ret_f ) 			//	если указатель на возврат передан, то возвоащаемся на него
		app_data->ret_f = app_data->proc->elf_finish;
	else					//	если нет, то на циферблат
		app_data->ret_f = show_watchface;
	
	// здесь проводим действия которые необходимо если функция запущена впервые из меню: заполнение всех структур данных и т.д.
	
	// первый кадр нулевой
	app_data->frame = 0;
	app_data->status = 0;

	set_hrm_mode(0x20); // однократный замер

}

// здесь выполняем отрисовку интерфейса, обновление (перенос в видеопамять) экрана выполнять не нужно
draw_frame();

set_display_state_value(8, 1);	//	выход из экрана 0 - разрешен; 1 - запрещен
set_display_state_value(4, 1);	//	подсветка принудительно включена 1, выключена 0


// заводим таймер на 10 секунд, если никаких действияствий не было выход из приложения
set_update_period(1, 200);
}

void key_press_screen(){
struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана


//	если запуск был из быстрого меню, при нажатии кнопки выходим на циферблат
if ( get_left_side_menu_active() ) 		
    app_data->proc->ret_f = show_watchface;
	
// вызываем функцию возврата (обычно это меню запуска), в качестве параметра указываем адрес функции нашего приложения
show_menu_animate(app_data->ret_f, (unsigned int)show_screen, ANIMATE_RIGHT);	
};

int dispatch_screen (void *param){
struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана


// в случае отрисовки интерфейса, обновление (перенос в видеопамять) экрана выполнять нужно

struct gesture_ *gest = param;
int result = 0;


switch (gest->gesture){
	case GESTURE_CLICK: {		
			break;
		};
		case GESTURE_SWIPE_RIGHT: 	//	свайп направо
		case GESTURE_SWIPE_LEFT: {	// справа налево
			
			
			if ( get_left_side_menu_active()){
					// в случае запуска через быстрое меню в proc->ret_f содержится dispatch_left_side_menu
					// и после отработки elf_finish (на который указывает app_data->ret_f, произойдет запуск dispatch_left_side_menu c
					// параметром структуры события тачскрина, содержащемся в app_data->proc->ret_param0
					
					// запускаем dispatch_left_side_menu с параметром param в результате произойдет запуск соответствующего бокового экрана
					// при этом произойдет выгрузка данных текущего приложения и его деактивация.
					void* show_f = get_ptr_show_menu_func();
					dispatch_left_side_menu(param);
										
					if ( get_ptr_show_menu_func() == show_f ){
						// если dispatch_left_side_menu вернет GESTURE_SWIPE_RIGHT то левее экрана нет, листать некуда
						// просто игнорируем этот жест
						return 0;
					}
					
					//	если dispatch_left_side_menu отработал, то завершаем наше приложение, т.к. данные экрана уже выгрузились
					// на этом этапе уже выполняется новый экран (тот куда свайпнули)
					Elf_proc_* proc = get_proc_by_addr(main);
					proc->ret_f = NULL;
					
					elf_finish(main);	//	выгрузить Elf из памяти
					return 0;
				} else { 			//	если запуск не из быстрого меню, обрабатываем свайпы по отдельности
					switch (gest->gesture){
						case GESTURE_SWIPE_RIGHT: {	//	свайп направо
							//	отключить измерение
							set_hrm_mode(0);
			
							return show_menu_animate(app_data->ret_f, (unsigned int)main, (gest->gesture == GESTURE_SWIPE_RIGHT) ? ANIMATE_RIGHT : ANIMATE_LEFT);
							break;
						}
						case GESTURE_SWIPE_LEFT: {	// справа налево
							//	действие при запуске из меню и дальнейший свайп влево
							
							
							break;
						}
					} /// switch (gest->gesture)
				}

			break;
		};	//	case GESTURE_SWIPE_LEFT:
		
			
		case GESTURE_SWIPE_UP: {	// свайп вверх
			
			break;
		};
		case GESTURE_SWIPE_DOWN: {	// свайп вниз
			break;
		};		
		default:{	// что-то пошло не так...
			
			break;
		};		
		
	}	//	switch (gest->gesture)
	
	return result;
};


void draw_frame(){
// при необходимости можно использовать данные экрана в этой функции
struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана

set_bg_color(COLOR_BLACK);
fill_screen_bg();

struct res_params_ res_params;	//	параметры графического реурса
int result;

result = get_res_params(app_data->proc->index_listed, app_data->frame, &res_params);
if (result) return;

result = show_elf_res_by_id(app_data->proc->index_listed, app_data->frame, (176-res_params.width)/2, (176-res_params.height)/2 );
if (result) return;

return;	
}

int screen_job(){
// при необходимости можно использовать данные экрана в этой функции
struct app_data_** 	app_data_p = get_ptr_temp_buf_2(); 	//	указатель на указатель на данные экрана 
struct app_data_ *	app_data = *app_data_p;				//	указатель на данные экрана

void* hrm_data = get_hrm_struct(); 
int heartrate;


if (get_fw_version() == NOT_LATIN_1_1_2_05){
	app_data->status = ((hrm_data_struct_legacy*)hrm_data)->ret_code;
	heartrate = ((hrm_data_struct_legacy*)hrm_data)->heart_rate;

} else {
	app_data->status = ((hrm_data_struct*)hrm_data)->ret_code;
	heartrate = ((hrm_data_struct*)hrm_data)->heart_rate;


}

switch (app_data->status){
	default:
	case 5:{		//	замер не завершен
		draw_frame();
		
		app_data->frame++;
		app_data->frame %= 14; 
		
		set_update_period(1, 100);
		break;
	}

	case 0:{		//	замер завершен успешно
		set_bg_color(COLOR_BLACK);
		fill_screen_bg();
		
		vibrate(1, 100, 0);
		
		set_fg_color(COLOR_WHITE);
		char text[10];
		_sprintf(text, "%d", heartrate);
		show_big_digit(3, text, 70, 60, 2); 	//	отображение цифр большим шрифтом
		
		set_display_state_value(4, 0);	//	подсветка принудительно включена 1, выключена 0
		set_update_period(0, 0);	
		break;
	} 

	case 2:{		//	замер завершен неуспешно
		set_bg_color(COLOR_BLACK);
		fill_screen_bg();
		
		set_fg_color(COLOR_WHITE);
		text_out_center("Не удалось\nзамерить пульс", 88, 70);
		
		set_display_state_value(4, 0);	//	подсветка принудительно включена 1, выключена 0
		set_update_period(0, 0);	
		break;
	}
	
}

repaint_screen_lines(0,176);

return 0;
}
#include "main.h"

#include "ui.h"
#include "basicui.h"
#include "EmbUI.h"
#include "interface.h"


/**
 * Define configuration variables and controls handlers
 * variables has literal names and are kept within json-configuration file on flash
 * 
 * Control handlers are bound by literal name with a particular method. This method is invoked
 * by manipulating controls
 * 
 */
void create_parameters(){
    LOG(println, F("Создание конфигурационных переменных по-умолчанию"));

    embui.var_create(FPSTR(P_hostname), "esp32cam");        // device hostname (autogenerated on first-run)
    BasicUI::add_sections();

    embui.section_handle_add("esp32cam", block_cam);

    embui.section_handle_add("ledBtn", led_toggle);
    embui.section_handle_add("ledBright", set_led_bright);
}

/**
 * This code builds UI section with menu block on the left
 * 
 */
void block_menu(Interface *interf, JsonObject *data){
    if (!interf) return;
    // создаем меню
    interf->json_section_menu();

    interf->option("esp32cam", F("esp32cam"));

    /**
     * добавляем в меню пункт - настройки,
     * это автоматически даст доступ ко всем связанным секциям с интерфейсом для системных настроек
     * 
     */
    BasicUI::opt_setup(interf, data);       // пункт меню "настройки"
    interf->json_section_end();
}

/**
 * Headlile section
 * this is an overriden weak method that builds our WebUI from the top
 * ==
 * Головная секция
 * переопределенный метод фреймфорка, который начинает строить корень нашего WebUI
 * 
 */
void section_main_frame(Interface *interf, JsonObject *data){
    if (!interf) return;

    interf->json_frame_interface("esp32camdemo");  // HEADLINE for WebUI
    block_menu(interf, data);

    if(!embui.sysData.wifi_sta){                // если контроллер не подключен к внешней AP, открываем вкладку настройки WiFi 
        BasicUI::block_settings_netw(interf, data);
    } else {
        block_cam(interf, data);
    }

    interf->json_frame_flush();
}

uint8_t ledbright = 127;

// обработчик кнопки "Переключение светодиода"
void led_toggle(Interface *interf, JsonObject *data){
    if (!interf || !data) return;
    if(ledcRead(1))
        ledcWrite(1, 0);
    else
        ledcWrite(1, ledbright);
}

void set_led_bright(Interface *interf, JsonObject *data){
    if (!interf || !data) return;
    ledbright = (*data)["ledBright"].as<int>();
    ledcWrite(1, ledbright);
}

void block_cam(Interface *interf, JsonObject *data){
    if (!interf) return;
    //LOG(println, "Here");

    interf->json_frame_interface();
    interf->json_section_main(String("esp32cam"), String("ESP32CAM"));

    //interf->frame("jpgframe", "<iframe class=\"iframe\" src=\"jpg\"></iframe>"); // marginheight=\"0\" marginwidth=\"0\" width=\"100%\" height=\"100%\" frameborder=\"0\" scrolling=\"yes\"
    //interf->frame2("jpgframe", "jpg");
    interf->frame2("mjpgframe", "mjpeg/1");
    interf->range("ledBright",ledbright,1,255,1,"Уровень светимости светодиода", true);
    interf->button("ledBtn","Переключение светодиода");
    
    interf->json_section_end();
    interf->json_frame_flush();
}




void pubCallback(Interface *interf){
    if (!interf) return;
    interf->json_frame_value();
    interf->value(F("pTime"), embui.timeProcessor.getFormattedShortTime(), true);
    interf->value(F("pMem"), String(ESP.getFreeHeap()), true);
    interf->value(F("pUptime"), String(millis()/1000), true);
    interf->json_frame_flush();
}

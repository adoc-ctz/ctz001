// コンパイル方法： $ gcc JJY_waveclock.c -o JJY_waveclcok2 -lwiringPi
//
// 入力パラメータ
// P1:JJY, P2:西暦年, P3:月, P4:日, P5:時, P6:分, P7:繰返し回数, P8:うるう秒
// (年・時刻の範囲チェック等は無し)
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h> /* wiringPi */
#include <signal.h>
#include <time.h> /* 時間関連定義引込み */

#define PIN RPI_V2_GPIO_P1_07

#define GPIO7 7

static struct timespec hrt_starttime;
static struct timespec hrt_endofmin;
static struct timespec hrt_now;
static struct timespec hrt_highduration;
static struct timespec hrt_lowduration;
static struct timespec hrt_debug;


// 一秒間パルス発生　ノイズ処理未
//
void gen_pulse(int ms)
{
    long ns = ms * 1000000;

    clock_gettime(CLOCK_MONOTONIC, &hrt_now);

    hrt_highduration = hrt_now;

    digitalWrite(GPIO7, 1);
    hrt_highduration.tv_nsec = hrt_now.tv_nsec + ns;

    if(hrt_highduration.tv_nsec >= 1000000000) {
       hrt_highduration.tv_nsec -= 1000000000;
       hrt_highduration.tv_sec = hrt_highduration.tv_sec + 1;
    }

//    printf("%02d %09d \n", hrt_now.tv_sec, hrt_now.tv_nsec); /* debug */
    clock_gettime(CLOCK_MONOTONIC, &hrt_debug); /* debug */
    printf("%02d %09d \n", hrt_debug.tv_sec, hrt_debug.tv_nsec); /* debug */
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &hrt_highduration, NULL);
//    clock_gettime(CLOCK_MONOTONIC, &hrt_now); /* debug */
//    printf("%02d %09d \n", hrt_now.tv_sec, hrt_now.tv_nsec); /* debug */
    digitalWrite(GPIO7, 0);
    clock_gettime(CLOCK_MONOTONIC, &hrt_debug); /* debug */
    printf("%02d %09d \n", hrt_debug.tv_sec, hrt_debug.tv_nsec); /* debug */
}


// 一分間のパルス発生
//
void transmit_timecode(int *send_data)
{
    int i, endofmin;

    clock_gettime(CLOCK_MONOTONIC, &hrt_lowduration);


    for( i = 0; i < 60; i++ ) {

    hrt_lowduration.tv_nsec = hrt_lowduration.tv_nsec + 999999999;
    if(hrt_lowduration.tv_nsec >= 1000000000) {
       hrt_lowduration.tv_nsec -= 1000000000;
       hrt_lowduration.tv_sec = hrt_lowduration.tv_sec + 1;
    }
    if(i == 59) {endofmin = 0;
    } else {endofmin = 1;
    }
        switch( send_data[i] ) {

            case -1:    // マーカー
                printf("m \n"); /* debug */
                gen_pulse(200);
                if(endofmin == 1) {
                  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &hrt_lowdurati
on, NULL);
                }
                break;

            case 1:     //　1
                printf("500 \n"); /* debug */
                gen_pulse(500);
                if(endofmin == 1) {
                 clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &hrt_lowduratio
n, NULL);
                }
                break;

            case 0:    //　0
                printf("800 \n"); /* debug */
                gen_pulse(800);
                if(endofmin == 1) {
                 clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &hrt_lowduratio
n, NULL);
                }
                break;
        }
    }
}


// 一分毎のタイミング合わせ
//
// void wait_for_next_minute()
// {
//     struct timeval tv;
//     time_t jt;
//     struct tm *tmt;
//     suseconds_t usec;
//
//     gettimeofday(&tv,NULL);
//     usec = (suseconds_t)1000000 - (tv.tv_usec % 1000000);
//     fprintf(stdout , "wait for next second, %u usec\n",  usec );
//     usleep(usec);
//
//     jt = time(NULL);
//     tmt = localtime(&jt);
//     usec = (60 - tmt->tm_sec) * 1000 * 1000;
//     fprintf(stdout , "wait for next minute, %u usec\n",  usec );
//     usleep(usec);
// }

void signal_handler(int sig)
{
    digitalWrite(GPIO7, 0);
    exit(0);
}


// 入力　仕様タイプ：c　yyyy mm dd hh mm　繰返し回数：xxxx うるう秒
//
int main(int argc, char *argv[])
{
    int i, l;
    double j, k;
    int f_daemon = 0;
    time_t  cal_time;
    struct tm *wtm;

    /* GPIO 初期設定 */
    if(wiringPiSetupGpio() == -1) {
       printf("error \n");
       return 1;
    }

    /* GPIO7 出力設定 */
    pinMode(GPIO7, OUTPUT);

    /* 入力時刻の表現形式変換 */
    /* タイプ　西暦年　月　日　時　分　(繰返し回数) うるう秒*/
    j = 1000;
    if( argc > 1 ) {
        for( i = 1; i < argc; i++ ) {
            char *param = argv[i];
            time_t  cal_time;
            char    *tstr;
        }
        cal_time = time(NULL);
        wtm = localtime(&cal_time);
        wtm->tm_year = atoi(argv[2]) - 1900;
        wtm->tm_mon = atoi(argv[3]) - 1;
        wtm->tm_mday = atoi(argv[4]);
        wtm->tm_hour = atoi(argv[5]);
        wtm->tm_min = atoi(argv[6]);
        wtm->tm_sec = 0;
        wtm->tm_isdst = 0; /* 日本では0 */

        /* 繰返し回数設定 */
        if( argc >= 7 ) {
            j = atol(argv[7]);
        /* うる秒 */
            l = 0;
            if(argc >= 8){
              l = atoi(argv[8]);
              if(l != 3){
                if(l != 2){
                  l = 0;
                }
              }
            }
        } else {
            j = 1000;
        }
        printf("%f \n", j); /* debug */

        /* 時刻変換 */
        if((cal_time = mktime(wtm)) == -1){
                printf("変換失敗\n");
                return(-1);
        }
        printf("変換結果 : %d\n", cal_time);
        printf("CTIME = %s", ctime(&cal_time));
//        return(0);

    } else {
        cal_time = time(NULL);
        wtm = localtime(&cal_time);
        j = 1000;
        l = 0;
        printf("%f\n",j); /* debug */
        printf("変換結果 : %d\n", cal_time);
        printf("CTIME = %s", ctime(&cal_time));
    }

    /* signal handle */
    signal( SIGKILL, signal_handler );
    signal( SIGTERM, signal_handler );
    signal( SIGINT, signal_handler );

    /* 開始時刻 */
    clock_gettime(CLOCK_MONOTONIC, &hrt_starttime);
    hrt_endofmin = hrt_starttime;
    hrt_endofmin.tv_sec = hrt_starttime.tv_sec + 60;

    printf("s %02d %09d \n", hrt_starttime.tv_sec, hrt_starttime.tv_nsec); /* de
bug */
    if( f_daemon ) daemon( 0, 0 );

    /* 繰返し回数分　または　終了コマンドまで繰返し */
    for( k = j; k > 0; k-- ) {
        int t;
        time_t jt;
        struct tm *tmt;
        int send_data[60];

        jt = time(NULL);
        tmt = wtm;
        fprintf(stdout, "Current time: %04d/%02d/%02d %02d:%02d:%02d\n",
        tmt->tm_year + 1900, tmt->tm_mon+1, tmt->tm_mday, tmt->tm_hour,
        tmt->tm_min, tmt->tm_sec) ;

        // マーカー
        send_data[0] = -1;

        // 分の10の位
        t = tmt->tm_min / 10;
        send_data[1] = t & 4 ? 1 : 0;
        send_data[2] = t & 2 ? 1 : 0;
        send_data[3] = t & 1 ? 1 : 0;
        send_data[4] = 0;

        // 分の1の位
        t = tmt->tm_min - t * 10;
        send_data[5] = t & 8 ? 1 : 0;
        send_data[6] = t & 4 ? 1 : 0;
        send_data[7] = t & 2 ? 1 : 0;
        send_data[8] = t & 1 ? 1 : 0;

        // マーカー
        send_data[9] = -1;
        send_data[10] = 0;
        send_data[11] = 0;

        // 時の10の位
        t = tmt->tm_hour / 10;
        send_data[12] = t & 2 ? 1 : 0;
        send_data[13] = t & 1 ? 1 : 0;
        send_data[14] = 0;

        // 時の1の位
        t = tmt->tm_hour - t * 10;
        send_data[15] = t & 8 ? 1 : 0;
        send_data[16] = t & 4 ? 1 : 0;
        send_data[17] = t & 2 ? 1 : 0;
        send_data[18] = t & 1 ? 1 : 0;

        // マーカー
        send_data[19] = -1;

        send_data[20] = 0;
        send_data[21] = 0;

        // 1月1日からの経過日100の位
        t = (tmt->tm_yday + 1) / 100;
        send_data[22] = t & 2 ? 1 : 0;
        send_data[23] = t & 1 ? 1 : 0;

        send_data[24] = 0;

        // 1月1日からの経過日の10の位
        t = (tmt->tm_yday + 1) / 10 - t * 10;
        send_data[25] = t & 8 ? 1 : 0;
        send_data[26] = t & 4 ? 1 : 0;
        send_data[27] = t & 2 ? 1 : 0;
        send_data[28] = t & 1 ? 1 : 0;

        // マーカー
        send_data[29] = -1;

        // 1月1日からの経過日の1の位
        t = (tmt->tm_yday + 1) % 100 - t * 10;
        send_data[30] = t & 8 ? 1 : 0;
        send_data[31] = t & 4 ? 1 : 0;
        send_data[32] = t & 2 ? 1 : 0;
        send_data[33] = t & 1 ? 1 : 0;

        send_data[34] = 0;
        send_data[35] = 0;

        // チェックサム
       t = 0;
        for(i = 12 ;i < 19 ; i++) {
            t += send_data[i] == 1 ? 1 : 0;
        }
        send_data[36]= t & 1;

        // チェックサム
       t = 0;
        for (i = 1 ; i < 9 ; i++) {
            t += send_data[i] == 1 ? 1 : 0;
        }
        send_data[37]= t & 1;

        send_data[38] = 0;

        // マーカー
        send_data[39] = -1;
        send_data[40] = 0;

        // 年の10の位
        t = (tmt->tm_year % 100) / 10;
        send_data[41]= t & 8 ? 1 : 0;
        send_data[42]= t & 4 ? 1 : 0;
        send_data[43]= t & 2 ? 1 : 0;
        send_data[44]= t & 1 ? 1 : 0;

        // 年の1の位
        t = (tmt->tm_year % 100 ) - t * 10;
        send_data[45] = t & 8 ? 1 : 0;
        send_data[46] = t & 4 ? 1 : 0;
        send_data[47] = t & 2 ? 1 : 0;
        send_data[48] = t & 1 ? 1 : 0;

        // マーカー
        send_data[49] = -1;

        // 週（曜日）
        t = tmt->tm_wday;
        send_data[50] = t & 4 ? 1 : 0;
        send_data[51] = t & 2 ? 1 : 0;
        send_data[52] = t & 1 ? 1 : 0;

        // うるう秒
        send_data[53] = l & 2 ? 1 : 0;
        send_data[54] = l & 1 ? 1 : 0;

        send_data[55] = 0;
        send_data[56] = 0;
        send_data[57] = 0;
        send_data[58] = 0;

        // マーカー
        send_data[59] = -1;

        transmit_timecode(send_data);

        wtm->tm_min = wtm->tm_min + 1;
        if((cal_time = mktime(wtm)) == -1){
            printf("変換失敗\n");
            return(-1);
        }

//        if(wtm->tm_min >= 60) {
//          wtm->tm_min = wtm->tm_min - 60;
//          wtm->tm_hour = wtm->tm_hour + 1;
//          if(wtm->tm_hour >= 13) {
//            wtm->tm_hour = wtm->tm_hour - 12;
//            wtm->tm_mday = wtm->tm_mday + 1;
//          }
//        }

        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &hrt_endofmin, NULL);
        printf("%02d %09d \n", hrt_endofmin.tv_sec, hrt_endofmin.tv_nsec); /* de
bug */
        hrt_endofmin.tv_sec = hrt_endofmin.tv_sec + 60;
   }
}
#define _GNU_SOURCE
#include <pthread.h>  // Thư viện quan trọng nhất để tạo và quản lý luồng (thread)
// ... (Các thư viện stdio, stdlib, unistd hỗ trợ in ấn, cấp phát, sleep)
#include <sched.h>    // Thư viện để can thiệp vào bộ lập lịch của CPU
#include <sys/sysinfo.h>
#include <sys/syscall.h>

#define BUFFER_SIZE 10 // Kích thước kho chứa (buffer) tối đa là 10 món hàng
#define MAX_THREAD 2   // Số lượng luồng tối đa là 2 (1 producer, 1 consumer)

int count = 0; // BIẾN QUAN TRỌNG NHẤT: Đếm số lượng hàng hóa đang có trong kho. Cả 2 luồng đều dùng chung biến này.
int in = 0;    // Con trỏ vị trí: Nơi Producer sẽ đặt hàng hóa tiếp theo vào kho.
int out = 0;   // Con trỏ vị trí: Nơi Consumer sẽ lấy hàng hóa tiếp theo ra khỏi kho.
int buffer[BUFFER_SIZE]; // Mảng số nguyên đóng vai trò làm kho chứa (Buffer).

void * producer(void * param) {
  // Chuyển tham số param (được truyền từ argv[1] lúc tạo luồng) từ dạng chuỗi (string) sang số nguyên (int). 
  // Biến max là tổng số hàng hóa cần sản xuất.
  int i = 0, max = atoi(param); 

  for (int i = 0; i < max; i++) {
    // BUSY WAITING: Nếu kho đầy (count == 10), thì chạy vòng lặp vô tận (do nothing) để chờ đợi.
    while (count == BUFFER_SIZE); 
    
    // Nếu kho chưa đầy, đặt số 1 (đại diện cho 1 món hàng) vào vị trí 'in' của mảng buffer.
    buffer[ in ] = 1; 

    // In ra màn hình vị trí vừa đặt và số lượng hàng đang có (trước khi tăng).
    printf("\nJust sent in = %d and count = %d.", in , count);
    printf("\nMon hang duoc dat tai = %d - tong hang trong kho = %d.", in , count);

    // Cập nhật vị trí 'in' tiếp theo. Phép chia lấy dư (%) giúp tạo ra mảng vòng (Circular Buffer). 
    // Ví dụ: in = 9, (9+1)%10 = 0. Nó sẽ quay lại đầu mảng thay vì tràn mảng.
    in = (in + 1) % BUFFER_SIZE; 

    // Tăng số lượng hàng hóa lên 1. (ĐÂY LÀ ĐIỂM GÂY RA LỖI RACE CONDITION).
    count++; 
  }
  
  // Sau khi sản xuất đủ 'max' món hàng, luồng tự kết thúc.
  pthread_exit(0); 
}

void * consumer(void * param) {
  int receive = 0; // Biến lưu tổng giá trị các món hàng đã lấy ra.
  
  while (1) { // Vòng lặp vô tận. Consumer sẽ liên tục đi tìm hàng để lấy.
    
    // BUSY WAITING: Nếu kho trống rỗng (count == 0), chạy vòng lặp vô tận (do nothing) để chờ Producer làm ra hàng.
    while (count == 0); 
    
    // Lấy hàng hóa (số 1) ở vị trí 'out' trong buffer cộng dồn vào biến receive.
    receive += buffer[out]; 
    
    // Giảm số lượng hàng hóa trong kho đi 1. (CŨNG LÀ ĐIỂM GÂY RA RACE CONDITION khi đụng độ với luồng kia).
    count--; 
    
    // In ra màn hình giá trị vừa nhận và số hàng còn lại.
    printf("\nSo mon hang da duoc ban = %d - tong hang trong kho = %d.", receive , count);
    
    // Cập nhật vị trí 'out' tiếp theo để lần sau lấy. Cũng dùng phép chia lấy dư để tạo mảng vòng.
    out = (out + 1) % BUFFER_SIZE; 
  }
  
  // Dòng này thực chất không bao giờ chạy tới được do kẹt ở vòng lặp while(1) phía trên. 
  // Nó chỉ kết thúc khi bị hàm main gọi pthread_cancel "giết" đi.
  pthread_exit(0); 
}

int main(int argc, char * argv[]) {
  pthread_t tid[MAX_THREAD]; /* Mảng lưu trữ ID của 2 luồng sắp tạo */

  // --- BƯỚC 1: Ép hệ điều hành ưu tiên chạy chương trình này ---
  struct sched_param sd;
  sd.sched_priority = 50; // Cài đặt mức độ ưu tiên theo thời gian thực (Real-time priority) là 50.
  sched_setscheduler(0, SCHED_RR, & sd); // Ép hệ điều hành dùng thuật toán lập lịch Round Robin (SCHED_RR) cho chương trình này.

  // --- BƯỚC 2: Ép chương trình chạy trên 2 lõi CPU cụ thể (CPU Affinity) ---
  cpu_set_t set;
  CPU_ZERO( & set);       // Khởi tạo một tập hợp các lõi CPU rỗng.
  CPU_SET(0, & set);      // Thêm lõi CPU số 0 vào tập hợp.
  CPU_SET(1, & set);      // Thêm lõi CPU số 1 vào tập hợp.
  // Lệnh dưới đây ép tiến trình hiện tại (getpid) chỉ được chạy trên các CPU đã set (lõi 0 và 1).
  // Mục đích: Để luồng Producer chạy ở lõi 0, Consumer chạy ở lõi 1, tranh chấp nhau biến `count` trong RAM, dễ sinh ra lỗi Race Condition.
  if (sched_setaffinity(getpid(), sizeof(set), & set) == -1) 
    printf("\nFailed to set affinity.");

  // --- BƯỚC 3: Tạo luồng ---
  // Tạo luồng Producer (tid[0]), chạy hàm producer(), truyền vào tham số argv[1] (số lượng hàng hóa muốn sản xuất từ dòng lệnh).
  pthread_create( & tid[0], NULL, producer, (argv[1]));
  // Tạo luồng Consumer (tid[1]), chạy hàm consumer().
  pthread_create( & tid[1], NULL, consumer, (argv[1]));

  // --- BƯỚC 4: Chờ đợi và kết thúc ---
  pthread_join(tid[0], NULL); // Hàm main sẽ tạm dừng ở đây để đợi luồng Producer (tid[0]) làm xong việc và tự thoát.
  sleep(1);                   // Ngủ 1 giây để đợi luồng Consumer lấy nốt những hàng hóa cuối cùng ra khỏi kho.
  pthread_cancel(tid[1]);     // Vì luồng Consumer dùng vòng lặp vô tận (while(1)), ta phải dùng lệnh này để "giết" nó đi.
  return 0;
}
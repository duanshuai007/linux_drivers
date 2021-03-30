#

`mem_rw_select.c`文件是基于内存读写的一个能够select的模型。

`tasklet.c`是一个基于内存读区的模型，不能select


| 标志 | 含义 |
| :-: | :-: |
|POLLIN | 如果设备无阻塞的读，就返回该值 |
| POLLRDNORM | 通常的数据已经准备好，可以读了，就返回该值。通常的做法是会返回(POLLLIN|POLLRDNORA) |
| POLLRDBAND | 如果可以从设备读出带外数据，就返回该值，它只可在linux内核的某些网络代码中使用，通常不用在设备驱动程序中|
| POLLPRI | 如果可以无阻塞的读取高优先级（带外）数据，就返回该值，返回该值会导致select报告文件发生异常，以为select八带外数据当作异常处理 |
| POLLHUP |  当读设备的进程到达文件尾时，驱动程序必须返回该值，依照select的功能描述，调用select的进程被告知进程时可读的。 |
| POLLERR |  如果设备发生错误，就返回该值。 |
| POLLOUT |  如果设备可以无阻塞地些，就返回该值 |
| POLLWRNORM | 设备已经准备好，可以写了，就返回该值。通常地做法是(POLLOUT|POLLNORR)|
| POLLWRBAND | 与POLLRDBAND类似 |

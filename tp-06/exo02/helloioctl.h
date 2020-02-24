#ifndef _HELLOIOCTL_H_
#define _HELLOIOCTL_H_

struct msg {
	char message[20];
};

#define	IOC_MAGIC	'N'
#define HELLO		_IOR(IOC_MAGIC, 0, struct msg)
#define WHO		_IOW(IOC_MAGIC, 1, struct msg)


#endif /* _HELLOIOCTL_H_ */
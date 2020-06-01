#include<iostream>
#include<cstdio>
#include<vector>
#include<ctime>
#include<cstring>
#include<unistd.h>
#include<cstdlib>
#include<pthread.h>
#define RESTYPE  100 // 资源的种类数
#define NTHREAD  50	// 线程的数目
using namespace std;

pthread_mutex_t mutex; // 互斥信号量
pthread_cond_t cond; // 条件变量

class BankerAlgorithm { // 银行家算法
public:
	int nthread; // 线程数
	int restThread; // 剩余正在执行的线程数目
	int nres; // 资源数
	int vis[NTHREAD]; // 标示这个进程有没有访问过
	int threadFinished[NTHREAD]; // 标示这个线程是否已经结束
	vector<int> resMax[NTHREAD]; // 每个线程对各类资源的最大的需求量
	vector<int> resAllocation[NTHREAD]; // 每个线程当前应经分配到各类资源的情况
	vector<int> resNeed[NTHREAD]; // 每个线程还需要每类资源的情况
	vector<int> resAvailable; // 各类资源的剩余可以利用的

private:
	void toNeed() {
		for (int i = 0; i < nthread; ++i)
			for (int j = 0; j < nres; ++j)
				resNeed[i].push_back(resMax[i][j]), resAllocation[i].push_back(0);
	}

	bool threadAafetyDetection(int idThread) {
		// 线程安全检测
		vector<int> tmpResAvailable(resAvailable);
		vector<int> threadSafeSequence; // 线程安全序列
		int cntThread = 0;
		memset(vis, 0, sizeof(vis));
		while (threadSafeSequence.size() < restThread) {
			bool findRunThread = false;
			for (int i = 0; i < nthread; ++i)
				if (!vis[i] && !threadFinished[i]) {
					int j;
					for (j = 0; j < nres; ++j)
						if (resNeed[i][j] > tmpResAvailable[j])
							break;
					if (j >= nres) {
						// 各类所需要的资源的数目 小于或等于各类剩余资源的数目
						// 该进程可以成功的运行完毕
						findRunThread = true;
						vis[i] = 1;
						threadSafeSequence.push_back(i);
						for (j = 0; j < nres; ++j)
							tmpResAvailable[j] += resAllocation[i][j];
					}
				}
			if (!findRunThread)
				break; // 找不到下一个可以运行的线程，则退出
		}

		if (threadSafeSequence.size() == restThread) {
			cout << "此时系统处于安全状态，存在线程安全序列如下:" << endl;
			for (int i = 0; i < threadSafeSequence.size(); ++i)
				cout << threadSafeSequence[i] << " ";
			cout << endl;
			return true;
		} else {
			cout << "此时系统处于不安全状态!!!资源无法分配!!!进程" << idThread << "将被阻塞!!!"
					<< endl; // 等到下一次resAvailable更新的时候再将该进程唤醒
			return false;
		}
	}

public:
	BankerAlgorithm() {
	}

	void init() {
		memset(threadFinished, 0, sizeof(threadFinished));
		// 初始化线程的数目， 资源种类的数目以及每种资源的数目
		cout << "请输入线程的数目和资源的种类数目:" << endl;
		cin >> nthread >> nres;
		restThread = nthread;
		cout << "请输入每种资源的数目:" << endl;
		for (int i = 0; i < nres; ++i) {
			int k;
			cin >> k;
			resAvailable.push_back(k);
		}

		cout << "请输入每个线程对某类资源最大的需求:" << endl;
		for (int i = 0; i < nthread; ++i) {
			cout << "线程" << i << "需要的资源:" << endl;
			for (int j = 0; j < nres; ++j) {
				int k;
				cin >> k;
				resMax[i].push_back(k);
			}
		}
		toNeed();
	}

	void returnRes(int idThread) {
		for (int i = 0; i < nres; ++i)
			resAvailable[i] += resAllocation[idThread][i], resAllocation[idThread][i] =
					0;
	}

	int bankerAlgorithm(int idThread, vector<int> res) { //进程idThread对资源idRes的请求数量为k
		for (int i = 0; i < res.size(); ++i) {
			int idRes = i, k = res[i];
			if (k <= resNeed[idThread][idRes]) {
				if (k > resAvailable[idRes]) {
					// 让进程阻塞
					cout << "ERROR!!!线程" << idThread << "请求" << idRes
							<< "类资源数目大于该类剩余资源的数目!" << endl << endl;
					return 1;
				}
			} else {                            //让进程重新请求资源
				cout << "ERROR!!!线程" << idThread << "请求" << idRes
						<< "类资源数目大于所需要的该类资源的数目!" << endl << endl;
				return 2;
			}
		}
		for (int i = 0; i < res.size(); ++i) {
			int idRes = i, k = res[i];
			resAvailable[idRes] -= k;
			resAllocation[idThread][idRes] += k;
			resNeed[idThread][idRes] -= k;
		}
		//安全性算法的检测
		if (!threadAafetyDetection(idThread)) {      //不能分配资源， 要将idThread这个线程阻塞
			for (int i = 0; i < res.size(); ++i) {
				int idRes = i, k = res[i];
				resAvailable[idRes] += k;
				resAllocation[idThread][idRes] -= k;
				resNeed[idThread][idRes] += k;
			}
			return 3;
		}
		cout << "线程" << idThread << "获得资源:";
		for (int i = 0; i < res.size(); ++i)
			cout << " " << i << "类:" << res[i];
		cout << endl << endl;
		return 0;
	}
};

BankerAlgorithm ba;

void* thread_hjzgg(void *arg) {
	long long idThread = (long long) arg; // 得到线程的标号
	srand((int) time(0));
	// 开始进行线程资源的请求
	vector<int> res;
	for (int i = 0; i < ba.nres; ++i) {
		int k = ba.resNeed[idThread][i] == 0 ?
				0 : rand() % ba.resNeed[idThread][i] + 1; // 线程对资源i申请的数目
		res.push_back(k);
	}
	while (1) {
		if (pthread_mutex_lock(&mutex) != 0) {
			cout << "线程" << idThread << "加锁失败!!!" << endl;
			pthread_exit(NULL);
		}

		bool isAllocationFinished = true; // 该线程是否已经将资源请求完毕
		for (int i = 0; i < ba.nres; ++i)
			if (ba.resNeed[idThread][i] != 0) {
				isAllocationFinished = false;
				break;
			}
		if (isAllocationFinished) {
			cout << "线程" << idThread << "资源分配完毕!!!进程得到想要的全部资源后开始继续执行!" << endl;
			cout << "................" << endl;
			sleep(1);
			cout << "线程" << idThread << "执行完毕!!!" << endl << endl;

			--ba.restThread;
			ba.threadFinished[idThread] = 1; // 线程结束
			ba.returnRes(idThread);
			pthread_cond_broadcast(&cond);
			pthread_mutex_unlock(&mutex);
			pthread_exit(NULL);
		}

		switch (ba.bankerAlgorithm(idThread, res)) {
		case 3: // 系统会进入不安全状态，不能进行资源的分配，先进行阻塞
		case 1: // 进程阻塞
			pthread_cond_wait(&cond, &mutex);
			break;
		case 2: // 重新分配资源
		case 0: // 资源分配成功, 接着在申请新的资源
			res.clear();
			for (int i = 0; i < ba.nres; ++i) {
				int k = ba.resNeed[idThread][i] == 0 ?
						0 : rand() % ba.resNeed[idThread][i] + 1; // 线程对资源i申请的数目
				res.push_back(k);
			}
			break;
		default:
			break;
		}
		sleep(1);
		pthread_mutex_unlock(&mutex);
	}
}

int main() {
	pthread_t tid[NTHREAD];
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	ba.init();
	for (int i = 0; i < ba.nthread; ++i)
		pthread_create(&tid[i], NULL, thread_hjzgg, (void*) i);

	for (int i = 0; i < ba.nthread; ++i)
		pthread_join(tid[i], NULL);
	return 0;
}

class GrabberThread
{
private:
    std::string device;
    std::mutex mtx;
    std::queue<cv::Mat > buffer;
    std::atomic<bool> grabOn;
    cv::VideoCapture cap;
    std::size_t bufSizeMax;
    std::map<unsigned char*, int> matMemoryCounter;
public:
    GrabberThread() : grabOn(false), device(""), bufSizeMax(0){};
    ~GrabberThread()
    {
        grabOn.store(false);
        cap.release();
        while (!buffer.empty())
            buffer.pop();
        matMemoryCounter.clear();
    }
    bool Init(std::string dev = "")
    {
        device = dev;
        cap.open(device);
        return cap.isOpened();
    }

    void StopGrabbing()
    {
        grabOn.store(false);
    }

    void GrabThread()
    {
        matMemoryCounter.clear();
        uchar * frameMemoryAddr;

        if (!cap.isOpened()) cap.open(device);
        if (!cap.isOpened()) return;

        cv::Mat tmp;
        grabOn.store(true);
        while (grabOn.load() == true)
        {
            if (!cap.grab())
                continue;
            mtx.lock();
            cap.retrieve(tmp);
            buffer.push(cv::Mat(tmp.size(), tmp.type()));
            tmp.copyTo(buffer.back());
            frameMemoryAddr = buffer.front().data;
            matMemoryCounter[frameMemoryAddr]++;
            bufSizeMax = std::max(bufSizeMax, buffer.size());

            mtx.unlock();
        } // while
    } // func

    bool PopFrame(cv::Mat &frame, std::size_t *pBufSize = 0, std::size_t *pBufSizeMax = 0)
    {
        mtx.lock();
        std::size_t bufSize = buffer.size();
        bool res = bufSize > 0;
        if (res) {
            // get the most recent grabbed frame (queue=LIFO)
            buffer.back().copyTo(frame);
            // release the item
            buffer.pop();
        }
        else frame = cv::Mat();

        if (pBufSize) *pBufSize = bufSize;
        if (pBufSizeMax) *pBufSizeMax = bufSizeMax;
        mtx.unlock();
        return res;
    }

    // GET SET METHODS
    /** see VideoCapture::set */
    virtual bool capSet(int propId, double value) {
        std::lock_guard<std::mutex> lock(mtx);
        return cap.set(propId, value);
    }
    /** see VideoCapture::get */
    virtual double capGet(int propId) {
        std::lock_guard<std::mutex> lock(mtx);
        return cap.get(propId);
    }
    std::size_t GetBufSize() {
        std::lock_guard<std::mutex> lock(mtx);
        return buffer.size();
    }
    std::size_t GetBufSizeMax() {
        std::lock_guard<std::mutex> lock(mtx);
        return bufSizeMax;
    }
    std::size_t GetMemoryCounter() {
        std::lock_guard<std::mutex> lock(mtx);
        return matMemoryCounter.size();
    }
    void PrintStats() {
        std::lock_guard<std::mutex> lock(mtx);
        std::cout << "DEVICE: " << device << std::endl;
        std::cout << "\tMaxBufSize: " << bufSizeMax << std::endl;
        std::cout << "\tcv::Mat Memory Counter: " << matMemoryCounter.size() << std::endl;
    }
}; //class Grabber
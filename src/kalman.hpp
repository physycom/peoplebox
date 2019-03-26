

class kalman_t
{
    int track_id_counter;
    std::chrono::steady_clock::time_point global_last_time;
    float dT;

public:
    int max_objects;    // max objects for tracking
    int min_frames;     // min frames to consider an object as detected
    const float max_dist;   // max distance (in px) to track with the same ID
    cv::Size img_size;  // max value of x,y,w,h

    struct tst_t {
        int track_id;
        int state_id;
        std::chrono::steady_clock::time_point last_time;
        int detection_count;
        tst_t() : track_id(-1), state_id(-1) {}
    };
    std::vector<tst_t> track_id_state_id_time;
    std::vector<bbox_t> result_vec_pred;

    struct one_kalman_t;
    std::vector<one_kalman_t> kalman_vec;

    struct one_kalman_t
    {
        cv::KalmanFilter kf;
        cv::Mat state;
        cv::Mat meas;
        int measSize, stateSize, contrSize;

        void set_delta_time(float dT) {
            kf.transitionMatrix.at<float>(2) = dT;
            kf.transitionMatrix.at<float>(9) = dT;
        }

        void set(bbox_t box)
        {
            initialize_kalman();

            kf.errorCovPre.at<float>(0) = 1; // px
            kf.errorCovPre.at<float>(7) = 1; // px
            kf.errorCovPre.at<float>(14) = 1;
            kf.errorCovPre.at<float>(21) = 1;
            kf.errorCovPre.at<float>(28) = 1; // px
            kf.errorCovPre.at<float>(35) = 1; // px

            state.at<float>(0) = box.x;
            state.at<float>(1) = box.y;
            state.at<float>(2) = 0;
            state.at<float>(3) = 0;
            state.at<float>(4) = box.w;
            state.at<float>(5) = box.h;
            // <<<< Initialization

            kf.statePost = state;
        }

        // Kalman.correct() calculates: statePost = statePre + gain * (z(k)-measurementMatrix*statePre);
        // corrected state (x(k)): x(k)=x'(k)+K(k)*(z(k)-H*x'(k))
        void correct(bbox_t box) {
            meas.at<float>(0) = box.x;
            meas.at<float>(1) = box.y;
            meas.at<float>(2) = box.w;
            meas.at<float>(3) = box.h;

            kf.correct(meas);

            bbox_t new_box = predict();
            if (new_box.w == 0 || new_box.h == 0) {
                set(box);
                //std::cerr << " force set(): track_id = " << box.track_id <<
                //    ", x = " << box.x << ", y = " << box.y << ", w = " << box.w << ", h = " << box.h << std::endl;
            }
        }

        // Kalman.predict() calculates: statePre = TransitionMatrix * statePost;
        // predicted state (x'(k)): x(k)=A*x(k-1)+B*u(k)
        bbox_t predict() {
            bbox_t box;
            state = kf.predict();

            box.x = state.at<float>(0);
            box.y = state.at<float>(1);
            box.w = state.at<float>(4);
            box.h = state.at<float>(5);
            return box;
        }

        void initialize_kalman()
        {
            kf = cv::KalmanFilter(stateSize, measSize, contrSize, CV_32F);

            // Transition State Matrix A
            // Note: set dT at each processing step!
            // [ 1 0 dT 0  0 0 ]
            // [ 0 1 0  dT 0 0 ]
            // [ 0 0 1  0  0 0 ]
            // [ 0 0 0  1  0 0 ]
            // [ 0 0 0  0  1 0 ]
            // [ 0 0 0  0  0 1 ]
            cv::setIdentity(kf.transitionMatrix);

            // Measure Matrix H
            // [ 1 0 0 0 0 0 ]
            // [ 0 1 0 0 0 0 ]
            // [ 0 0 0 0 1 0 ]
            // [ 0 0 0 0 0 1 ]
            kf.measurementMatrix = cv::Mat::zeros(measSize, stateSize, CV_32F);
            kf.measurementMatrix.at<float>(0) = 1.0f;
            kf.measurementMatrix.at<float>(7) = 1.0f;
            kf.measurementMatrix.at<float>(16) = 1.0f;
            kf.measurementMatrix.at<float>(23) = 1.0f;

            // Process Noise Covariance Matrix Q - result smoother with lower values (1e-2)
            // [ Ex   0   0     0     0    0  ]
            // [ 0    Ey  0     0     0    0  ]
            // [ 0    0   Ev_x  0     0    0  ]
            // [ 0    0   0     Ev_y  0    0  ]
            // [ 0    0   0     0     Ew   0  ]
            // [ 0    0   0     0     0    Eh ]
            //cv::setIdentity(kf.processNoiseCov, cv::Scalar(1e-3));
            kf.processNoiseCov.at<float>(0) = 1e-2;
            kf.processNoiseCov.at<float>(7) = 1e-2;
            kf.processNoiseCov.at<float>(14) = 1e-2;// 5.0f;
            kf.processNoiseCov.at<float>(21) = 1e-2;// 5.0f;
            kf.processNoiseCov.at<float>(28) = 5e-3;
            kf.processNoiseCov.at<float>(35) = 5e-3;

            // Measures Noise Covariance Matrix R - result smoother with higher values (1e-1)
            cv::setIdentity(kf.measurementNoiseCov, cv::Scalar(1e-1));

            //cv::setIdentity(kf.errorCovPost, cv::Scalar::all(1e-2));
            // <<<< Kalman Filter

            set_delta_time(0);
        }


        one_kalman_t(int _stateSize = 6, int _measSize = 4, int _contrSize = 0) :
            kf(_stateSize, _measSize, _contrSize, CV_32F), measSize(_measSize), stateSize(_stateSize), contrSize(_contrSize)
        {
            state = cv::Mat(stateSize, 1, CV_32F);  // [x,y,v_x,v_y,w,h]
            meas = cv::Mat(measSize, 1, CV_32F);    // [z_x,z_y,z_w,z_h]
            //cv::Mat procNoise(stateSize, 1, type)
            // [E_x,E_y,E_v_x,E_v_y,E_w,E_h]

            initialize_kalman();
        }
    };
    // ------------------------------------------



    kalman_t(int _max_objects = 1000, int _min_frames = 3, float _max_dist = 40, cv::Size _img_size = cv::Size(10000, 10000)) :
        max_objects(_max_objects), min_frames(_min_frames), max_dist(_max_dist), img_size(_img_size),
        track_id_counter(0)
    {
        kalman_vec.resize(max_objects);
        track_id_state_id_time.resize(max_objects);
        result_vec_pred.resize(max_objects);
    }

    float calc_dt() {
        dT = std::chrono::duration<double>(std::chrono::steady_clock::now() - global_last_time).count();
        return dT;
    }

    static float get_distance(float src_x, float src_y, float dst_x, float dst_y) {
        return sqrtf((src_x - dst_x)*(src_x - dst_x) + (src_y - dst_y)*(src_y - dst_y));
    }

    void clear_old_states() {
        // clear old bboxes
        for (size_t state_id = 0; state_id < track_id_state_id_time.size(); ++state_id)
        {
            float time_sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - track_id_state_id_time[state_id].last_time).count();
            float time_wait = 0.5;    // 0.5 second
            if (track_id_state_id_time[state_id].track_id > -1)
            {
                if ((result_vec_pred[state_id].x > img_size.width) ||
                    (result_vec_pred[state_id].y > img_size.height))
                {
                    track_id_state_id_time[state_id].track_id = -1;
                }

                if (time_sec >= time_wait || track_id_state_id_time[state_id].detection_count < 0) {
                    //std::cerr << " remove track_id = " << track_id_state_id_time[state_id].track_id << ", state_id = " << state_id << std::endl;
                    track_id_state_id_time[state_id].track_id = -1; // remove bbox
                }
            }
        }
    }

    tst_t get_state_id(bbox_t find_box, std::vector<bool> &busy_vec)
    {
        tst_t tst;
        tst.state_id = -1;

        float min_dist = std::numeric_limits<float>::max();

        for (size_t i = 0; i < max_objects; ++i)
        {
            if (track_id_state_id_time[i].track_id > -1 && result_vec_pred[i].obj_id == find_box.obj_id && busy_vec[i] == false)
            {
                bbox_t pred_box = result_vec_pred[i];

                float dist = get_distance(pred_box.x, pred_box.y, find_box.x, find_box.y);

                float movement_dist = std::max(max_dist, static_cast<float>(std::max(pred_box.w, pred_box.h)) );

                if ((dist < movement_dist) && (dist < min_dist)) {
                    min_dist = dist;
                    tst.state_id = i;
                }
            }
        }

        if (tst.state_id > -1) {
            track_id_state_id_time[tst.state_id].last_time = std::chrono::steady_clock::now();
            track_id_state_id_time[tst.state_id].detection_count = std::max(track_id_state_id_time[tst.state_id].detection_count + 2, 10);
            tst = track_id_state_id_time[tst.state_id];
            busy_vec[tst.state_id] = true;
        }
        else {
            //std::cerr << " Didn't find: obj_id = " << find_box.obj_id << ", x = " << find_box.x << ", y = " << find_box.y <<
            //    ", track_id_counter = " << track_id_counter << std::endl;
        }

        return tst;
    }

    tst_t new_state_id(std::vector<bool> &busy_vec)
    {
        tst_t tst;
        // find empty cell to add new track_id
        auto it = std::find_if(track_id_state_id_time.begin(), track_id_state_id_time.end(), [&](tst_t &v) { return v.track_id == -1; });
        if (it != track_id_state_id_time.end()) {
            it->state_id = it - track_id_state_id_time.begin();
            //it->track_id = track_id_counter++;
            it->track_id = 0;
            it->last_time = std::chrono::steady_clock::now();
            it->detection_count = 1;
            tst = *it;
            busy_vec[it->state_id] = true;
        }

        return tst;
    }

    std::vector<tst_t> find_state_ids(std::vector<bbox_t> result_vec)
    {
        std::vector<tst_t> tst_vec(result_vec.size());

        std::vector<bool> busy_vec(max_objects, false);

        for (size_t i = 0; i < result_vec.size(); ++i)
        {
            tst_t tst = get_state_id(result_vec[i], busy_vec);
            int state_id = tst.state_id;
            int track_id = tst.track_id;

            // if new state_id
            if (state_id < 0) {
                tst = new_state_id(busy_vec);
                state_id = tst.state_id;
                track_id = tst.track_id;
                if (state_id > -1) {
                    kalman_vec[state_id].set(result_vec[i]);
                    //std::cerr << " post: ";
                }
            }

            //std::cerr << " track_id = " << track_id << ", state_id = " << state_id <<
            //    ", x = " << result_vec[i].x << ", det_count = " << tst.detection_count << std::endl;

            if (state_id > -1) {
                tst_vec[i] = tst;
                result_vec_pred[state_id] = result_vec[i];
                result_vec_pred[state_id].track_id = track_id;
            }
        }

        return tst_vec;
    }

    std::vector<bbox_t> predict()
    {
        clear_old_states();
        std::vector<bbox_t> result_vec;

        for (size_t i = 0; i < max_objects; ++i)
        {
            tst_t tst = track_id_state_id_time[i];
            if (tst.track_id > -1) {
                bbox_t box = kalman_vec[i].predict();

                result_vec_pred[i].x = box.x;
                result_vec_pred[i].y = box.y;
                result_vec_pred[i].w = box.w;
                result_vec_pred[i].h = box.h;

                if (tst.detection_count >= min_frames)
                {
                    if (track_id_state_id_time[i].track_id == 0) {
                        track_id_state_id_time[i].track_id = ++track_id_counter;
                        result_vec_pred[i].track_id = track_id_counter;
                    }

                    result_vec.push_back(result_vec_pred[i]);
                }
            }
        }
        //std::cerr << "         result_vec.size() = " << result_vec.size() << std::endl;

        //global_last_time = std::chrono::steady_clock::now();

        return result_vec;
    }


    std::vector<bbox_t> correct(std::vector<bbox_t> result_vec)
    {
        calc_dt();
        clear_old_states();

        for (size_t i = 0; i < max_objects; ++i)
            track_id_state_id_time[i].detection_count--;

        std::vector<tst_t> tst_vec = find_state_ids(result_vec);

        for (size_t i = 0; i < tst_vec.size(); ++i) {
            tst_t tst = tst_vec[i];
            int state_id = tst.state_id;
            if (state_id > -1)
            {
                kalman_vec[state_id].set_delta_time(dT);
                kalman_vec[state_id].correct(result_vec_pred[state_id]);
            }
        }

        result_vec = predict();

        global_last_time = std::chrono::steady_clock::now();

        return result_vec;
    }

};

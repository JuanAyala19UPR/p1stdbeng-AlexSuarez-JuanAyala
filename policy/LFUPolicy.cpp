//
// Created by alex on 9/22/26.
//

#include "LFUPolicy.h"

namespace bufman {
    void LFUPolicy::init(std::size_t pool_size) {
        buckets_.clear();
        slots_.clear();
        min_freq_=0;
    }
    void LFUPolicy::on_access(std::size_t frame) {
        auto slot=slots_.find(frame);
        size_t& count=slot->second.count;
        auto& iterator=slot->second.pos;
        auto bucket=buckets_.find(count);
        //auto& list= bucket->second;
        bucket->second.erase(iterator);
        if (bucket->second.empty()) {
            buckets_.erase(bucket);
            if (count==min_freq_) {
                min_freq_=count+1;
            }
        }

        count+=1;
        //buckets_[count].push_front(frame);
        auto& bucket_new=buckets_[count];
        bucket_new.push_front(frame);
        iterator=bucket_new.begin();
    }
    void LFUPolicy::on_load(std::size_t frame) {
        auto& bucket=buckets_[1];
        bucket.push_front(frame);
        auto& slot=slots_[frame];
        slot.count=1;
        slot.pos=bucket.begin();
        min_freq_=1;
    }
    void LFUPolicy::on_remove(std::size_t frame) {
        auto slot=slots_.find(frame);
        //auto iterator=slot->second.pos;
        size_t& count=slot->second.count;
        auto bucket=buckets_.find(count);
        bucket->second.remove(frame);
        if (bucket->second.empty()) {
            buckets_.erase(bucket);
            if (!buckets_.empty()) {
                std::size_t minor=buckets_.begin()->first;
                for (auto i=buckets_.begin();i!=buckets_.end();i++) {
                    if (i->first<minor) {
                        minor=i->first;
                    }
                }
                min_freq_=minor;
            }else {
                min_freq_=0;
            }

        }
        slots_.erase(slot);
    }
    std::optional<std::size_t> LFUPolicy::pick_victim(
        const std::vector<std::size_t>& candidates) const {
        //auto candidate_menor=*candidates.begin();
        //auto candidate_menor = std::min_element(candidates.begin(), candidates.end());
        //Get frequence of first element of candeidates
        if (candidates.empty()) {
            return std::nullopt;
        }

        auto slot_menor=slots_.find(*candidates.begin());
        auto cantidad_menor=slot_menor->second.count;
        //auto frequence_menor=buckets_.find(cantidad_menor)->first;
        //auto selected_candidate=*candidates.begin();
        for (std::size_t i=0;i<candidates.size();i++) {
            //Get frequence of each candidate and compare with frequence menor
            auto slot=slots_.find(candidates[i]);
            auto cantidad=slot->second.count;
            //auto bucket=buckets_.find(cantidad);
            if (cantidad<cantidad_menor) {
                cantidad_menor=cantidad;
                //selected_candidate=candidates[i];
            }
        }
        auto bucket=buckets_.find(cantidad_menor);
        //std::vector<size_t>  selected_candidates;
        //bucket->second.erase(selected_candidate);
        /*for (std::size_t i=0;i<candidates.size();i++) {
            auto slot=slots_.find(candidates[i]);
            auto candidate= slot->second.count;
            if (frequence_menor==candidate) {
                selected_candidates.push_back(candidates[i]);
            }

        }*/
        //Must found it the first to
        for (auto i=bucket->second.rbegin();i!=bucket->second.rend();i++) {
            if (std::find(candidates.begin(), candidates.end(), *i) != candidates.end()) {
                return *i;
            }
        }
        return std::nullopt;
    }
}
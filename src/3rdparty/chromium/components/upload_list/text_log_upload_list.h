// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UPLOAD_LIST_TEXT_LOG_UPLOAD_LIST_H_
#define COMPONENTS_UPLOAD_LIST_TEXT_LOG_UPLOAD_LIST_H_

#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "components/upload_list/upload_list.h"

// Loads and parses an upload list text file of the line-based JSON format:
// {"upload_time":<value>[,"upload_id":<value>[,"local_id":<value>
// [,"capture_time":<value>[,"state":<value>[,"source":<value>]]]]]}
// {"upload_time":<value>[,"upload_id":<value>[,"local_id":<value>
// [,"capture_time":<value>[,"state":<value>[,"source":<value>]]]]]}
// ...
// or the CSV format:
// upload_time,upload_id[,local_id[,capture_time[,state]]]
// upload_time,upload_id[,local_id[,capture_time[,state]]]
// ...
// where each line represents an upload. |upload_time| and |capture_time| are in
// Unix time. |state| is an int in the range of UploadInfo::State. A line may
// or may not contain |local_id|, |capture_time|, |state| and |source|.
class TextLogUploadList : public UploadList {
 public:
  // Creates a new upload list that parses the log at |upload_log_path|.
  explicit TextLogUploadList(const base::FilePath& upload_log_path);

  TextLogUploadList(const TextLogUploadList&) = delete;
  TextLogUploadList& operator=(const TextLogUploadList&) = delete;

  const base::FilePath& upload_log_path() const { return upload_log_path_; }

 protected:
  ~TextLogUploadList() override;

  // UploadList:
  std::vector<UploadList::UploadInfo> LoadUploadList() override;
  void ClearUploadList(const base::Time& begin, const base::Time& end) override;
  void RequestSingleUpload(const std::string& local_id) override;

  // Parses upload log lines, converting them to UploadInfo entries.
  // The method also reverse the order of the entries (the first entry in
  // |uploads| is the last in |log_entries|).
  void ParseLogEntries(const std::vector<std::string>& log_entries,
                       std::vector<UploadInfo>* uploads);

  const base::FilePath upload_log_path_;
};

#endif  // COMPONENTS_UPLOAD_LIST_TEXT_LOG_UPLOAD_LIST_H_

(import build/curl :as curl)

(if (= nil (dyn :curl-easy-handle))
  (error "missing default value for curl-easy-handle"))

(let [outer-easy-handle (curl/easy-handle)]
  (resume
    (fiber/new (fn []
                 (if (not= nil (curl/easy-handle))
                   (error "unexpected Fiber curl-easy-handle"))
                 (curl/with-easy-handle
                   (if (= outer-easy-handle (curl/easy-handle))
                     (error "easy handle is not properly fiber scoped")))))))

(try
  (-> (curl/easy-handle) (curl/reset))
  ([err] (error "failed to reset easy handle: " err)))

(try
  (do
    (:set-opt (curl/easy-handle) :bad-option nil)
    (error "unhandled easy handle set-opt key"))
  ([err]))

# setopt

# invalid value type
(try 
  (do (-> (curl/easy-handle)
          (curl/set-opt :CURLOPT_PORT :foo))
    (error "expected set-opt error"))
  ([err]))

# string value

(-> (curl/easy-handle)
    (:set-opt :CURLOPT_URL "https://janet-lang.org"))

(curl/reset (curl/easy-handle))

# buffer value

(-> (curl/easy-handle)
    (:set-opt :CURLOPT_URL @"https://janet-lang.org"))

(curl/reset (curl/easy-handle))

# long value
(-> (curl/easy-handle)
    (:set-opt :CURLOPT_PORT 9999))

(curl/reset (curl/easy-handle))

# function values
(-> (curl/easy-handle)
    (:set-opt-function 
      :CURLOPT_READFUNCTION 
      :CURLOPT_READDATA 
      (fn [buf size nitems]
        (pp @[buf size nitems])
        0)))

(curl/reset (curl/easy-handle))

# write callback

(do
  (var called false)
  (-> (curl/easy-handle)
      (:set-opt :CURLOPT_URL "https://example.com")
      (:set-opt-function 
        :CURLOPT_WRITEFUNCTION :CURLOPT_WRITEDATA 
        (fn [buf size nitems]
          (set called true)
          (* size nitems)))
      (:perform))
  (unless called
    (error "write callback was not called")))

# header callback

(do
  (var called false)
  (-> (curl/easy-handle)
      (:set-opt :CURLOPT_URL "https://example.com")
      (:set-opt-function 
        :CURLOPT_HEADERFUNCTION :CURLOPT_HEADERDATA 
        (fn [buf size nitems]
          (set called true)
          (* size nitems)))
      (:perform))
  (unless called
    (error "write callback was not called")))


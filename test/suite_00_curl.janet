(import build/curl :as curl)

(curl/init)

(if (not= (curl/easy-handle) (curl/easy-handle))
  (error "Different CURL easy handles returned for the same fiber"))

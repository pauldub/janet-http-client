(import build/curl :as curl)

(curl/init)

(if (not= (curl/easy-handle) (curl/easy-handle))
  (error "Different CURL easy handles returned for the same fiber"))

(let [outer-handle (curl/easy-handle)
      f (fiber/new (fn [] (if (= outer-handle (curl/easy-handle))
                            (error "Same handle for two fibers"))))]
  (resume f))
  

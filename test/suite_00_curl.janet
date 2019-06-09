(import build/curl :as curl)

(if (= nil (dyn :curl-easy-handle))
  (error "Missing default value for curl-easy-handle"))

(let [outer-easy-handle (curl/easy-handle)]
  (resume
    (fiber/new (fn []
                 (if (not= nil (curl/easy-handle))
                   (error "Unexpected Fiber curl-easy-handle"))
                 (curl/with-easy-handle
                   (if (= outer-easy-handle (curl/easy-handle))
                     (error "Easy handle is not properly fiber scoped")))))))

(try
  (-> (curl/easy-handle) (curl/reset))
  ([err] (error "Failed to reset easy handle: " err)))

(try
  (do
    (:set-opt (curl/easy-handle) :bad-option nil)
    (error "unhandled easy handle set-opt key"))
  ([err]))

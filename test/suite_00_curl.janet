(import build/curl :as curl)

(if (= nil (dyn :curl-easy-handle))
  (error "Missing default value for curl-easy-handle"))

(let [outer-easy-handle (dyn :curl-easy-handle)]
  (resume
    (fiber/new (fn []
                 (curl/with-easy-handle
                   (if (= outer-easy-handle (dyn :curl-easy-handle))
                     (error "Easy handle is not properly fiber scoped")))))))

(try
  (do
    (:set-opt (dyn :curl-easy-handle) :bad-option nil)
    (error "unhandled easy handle set-opt key"))
  ([err]))

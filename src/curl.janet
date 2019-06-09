(defn easy-handle []
  "Helper function to get current value of dynamic variable `:curl-easy-handle`."
  (dyn :curl-easy-handle))

(defmacro with-easy-handle [& body]
  "Wraps body with a fiber scoped CURL easy_handle."
  (with-syms [h]
    ~(let [,h (or ,(dyn :curl-easy-handle) 
                  (,make-easy-handle))]
       (with-dyns [:curl-easy-handle ,h]
         ,;body))))



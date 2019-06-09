(def request-map 
  { :headers {}
    :body false
    :query-string false
    :scheme :http
    :protocol "HTTP/1.1"
    :method false })

(def response-map
  { :status false
    :headers {}
    :body [] })

(defn get [url &keys options]
  "Sends a GET request to `url` with request `options`."
  )
